import React, { useCallback, useEffect, useState } from 'react'
import AppDialog from '@/common/components/appDialog';
import ConfirmDialog from '@/common/components/confirmDialog';
import {
    Box,
    Button,
    CircularProgress,
    LinearProgress,
    useMediaQuery,
    useTheme,
} from '@mui/material'
import { TransitionProps } from '@mui/material/transitions'
import { visuallyHidden } from '@mui/utils'
import Slide from '@mui/material/Slide'
import Zoom from '@mui/material/Zoom'
import CloudUploadIcon from '@mui/icons-material/CloudUpload'
import uuid from 'react-uuid'

import { useAppDispatch } from '@/store'
import { addAlertSnackbar } from '@/store/snackbars/snackbars.Slice'
import { Area, Point } from 'react-easy-crop'
import { DEFAULT_FILL_COLOR, FillColor, getCroppedImg } from './imageCropping/cropper/utils'
import ImageCropping from './imageCropping/imageCropping'
import ImageAdjust from './imageAdjust/imageAdjust'
import { Adjustment, NEUTRAL_ADJUSTMENT, isNeutral } from './imageAdjust/adjust'
import { adjustImage } from './imageAdjust/adjustImage'
import ImagePreview from './imagePreview/imagePreview'
import { useBlobImage } from './useBlobImage'
import EpaperToggle, { EpaperView } from './epaper/epaperToggle'
import { useUploadQueue } from './useUploadQueue'
import { usePhotoPicker } from './usePhotoPicker'
import EditorLayout, { EditorGrid } from './editorLayout'
import StartScreen from './startScreen'
import ThumbStrip from './thumbStrip'
import ToolTabs, { Tool } from './toolTabs'

const ZoomTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<unknown>
    },
    ref: React.Ref<unknown>
) {
    return <Zoom ref={ref} {...props} />
})

const SlideTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<unknown>
    },
    ref: React.Ref<unknown>
) {
    return <Slide ref={ref} direction="up" {...props} />
})

const CROPPER_PROPS_INITIAL_STATE = {
    crop: { x: 0, y: 0 },
    zoom: 1,
    rotation: 0,
    croppedAreaPixels: { x: 0, y: 0, width: 0, height: 0 }
}

export interface PictureUploadDialogProps {
    open: boolean;
    onClose: () => void;
    /** Takes one finished picture; resolves false when the board refused it, so it stays in the editor. */
    onUpload: (file: File) => Promise<boolean>;
    /** How many photos fit into this session. */
    maxFiles: number;
    uploading: boolean;
    title?: string;
}

const PictureUploadDialog: React.FC<PictureUploadDialogProps> = ({
    open,
    onClose,
    onUpload,
    maxFiles,
    uploading,
    title: dialogTitle = 'Fotos hochladen',
}) => {
    const theme = useTheme();
    const dispatch = useAppDispatch();
    const matches = useMediaQuery(theme.breakpoints.up('md'));
    const showCamera = useMediaQuery('(pointer: coarse)');

    const {
        imageStatuses,
        setImageStatuses,
        addImages,
        reset: resetQueue,
        imagePreviews,
        previewErrors,
        markPreviewBroken,
    } = useUploadQueue(maxFiles)
    const { inputProps, openPicker, selectFiles, isPreparing } = usePhotoPicker(imageStatuses, addImages, maxFiles)

    const [tool, setTool] = useState<Tool>('crop')
    const [previewing, setPreviewing] = useState(false)
    const [busy, setBusy] = useState(false)
    // The cropper measures itself on mount, which mid-transition yields a scaled-down box.
    const [entered, setEntered] = useState(false)
    const [currentImageIndex, setCurrentImageIndex] = useState<number | null>(null);

    const [croppedAreaPixels, setCroppedAreaPixels] = useState<Area>(CROPPER_PROPS_INITIAL_STATE.croppedAreaPixels)
    const [crop, setCrop] = useState<Point>(CROPPER_PROPS_INITIAL_STATE.crop)
    const [zoom, setZoom] = useState(CROPPER_PROPS_INITIAL_STATE.zoom)
    const [rotation, setRotation] = useState(CROPPER_PROPS_INITIAL_STATE.rotation)
    const [fillColor, setFillColor] = useState<FillColor>(DEFAULT_FILL_COLOR)
    const [croppedImage, replaceCroppedImage] = useBlobImage()
    const [croppedKey, setCroppedKey] = useState<string | null>(null)
    const [adjustment, setAdjustment] = useState<Adjustment>(NEUTRAL_ADJUSTMENT)
    const [finalImage, replaceFinalImage] = useBlobImage()
    const [epaperView, setEpaperView] = useState<EpaperView>('original')
    const [confirmDiscard, setConfirmDiscard] = useState(false)
    const [removeId, setRemoveId] = useState<string | null>(null)

    // Everything derived from the photo in edit belongs to that photo only.
    const resetCropperState = useCallback(() => {
        setCroppedAreaPixels(CROPPER_PROPS_INITIAL_STATE.croppedAreaPixels)
        setCrop(CROPPER_PROPS_INITIAL_STATE.crop)
        setZoom(CROPPER_PROPS_INITIAL_STATE.zoom)
        setRotation(CROPPER_PROPS_INITIAL_STATE.rotation)
        setFillColor(DEFAULT_FILL_COLOR)
        replaceCroppedImage(null)
        setCroppedKey(null)
        setAdjustment(NEUTRAL_ADJUSTMENT)
        replaceFinalImage(null)
        setEpaperView('original')
        setPreviewing(false)
        setTool('crop')
    }, [replaceCroppedImage, replaceFinalImage])

    const closeDialog = () => {
        setConfirmDiscard(false);
        setRemoveId(null);
        onClose();
        resetQueue();
        setCurrentImageIndex(null);
        resetCropperState();
    }

    // Closing by hand (X, Escape, back) would drop every photo still being edited.
    const handleDialogClose = () => {
        if (!busy && !uploading && imageStatuses.some(status => status.status !== 'uploaded')) {
            setConfirmDiscard(true)
            return
        }
        closeDialog()
    }

    const selectImageForCropping = useCallback((index: number) => {
        if (imageStatuses[index].status === 'uploaded') return

        resetCropperState()
        setCurrentImageIndex(index)
        setImageStatuses(prev =>
            prev.map((status, i) => ({
                ...status,
                status: i === index ? 'cropping' : (status.status === 'cropping' ? 'pending' : status.status)
            }))
        )
    }, [imageStatuses, resetCropperState, setImageStatuses])

    const handleSelectPhoto = (index: number) => {
        if (index === currentImageIndex || busy) return
        selectImageForCropping(index)
    }

    const removeImage = (indexToRemove: number) => {
        const remaining = imageStatuses.filter((_, index) => index !== indexToRemove)
        if (remaining.length > 0 && remaining.every(status => status.status === 'uploaded')) {
            closeDialog()
            return
        }
        setImageStatuses(remaining)
        if (indexToRemove === currentImageIndex) {
            resetCropperState()
            setCurrentImageIndex(null)
        } else {
            setCurrentImageIndex(prev => (prev !== null && indexToRemove < prev ? prev - 1 : prev))
        }
    }

    const removeIndex = imageStatuses.findIndex(status => status.id === removeId)
    const confirmRemove = () => {
        setRemoveId(null)
        if (removeIndex !== -1) removeImage(removeIndex)
    }

    const currentImage = currentImageIndex !== null ? imageStatuses[currentImageIndex] ?? null : null
    const area = croppedAreaPixels
    const cropKey = currentImage ? `${currentImage.id}:${area.x},${area.y},${area.width},${area.height}:${rotation}:${fillColor}` : null
    const cropMeasured = area.width >= 1 && area.height >= 1
    const editLocked = busy || !cropMeasured

    // The crop is computed when a later tool needs it and reused until its inputs change.
    const ensureCropped = async (): Promise<Blob> => {
        if (croppedImage && croppedKey === cropKey) return croppedImage.blob
        if (!currentImage || area.width < 1 || area.height < 1) {
            throw new Error('Der Zuschnitt wird noch vorbereitet. Bitte kurz warten.')
        }
        const blob = await getCroppedImg(currentImage.file, area, rotation, fillColor)
        replaceCroppedImage(blob)
        setCroppedKey(cropKey)
        return blob
    }

    const runBusy = async (task: () => Promise<void>) => {
        setBusy(true)
        try {
            await task()
        } catch (error) {
            dispatch(addAlertSnackbar(uuid(), error instanceof Error ? error.message : 'Bearbeiten fehlgeschlagen', 'error'))
        } finally {
            setBusy(false)
        }
    }

    const changeTool = (next: Tool) => {
        if (next === 'crop') {
            setTool(next)
            return
        }
        runBusy(async () => {
            await ensureCropped()
            setTool(next)
        })
    }

    const goToPreview = () => runBusy(async () => {
        const cropped = await ensureCropped()
        replaceFinalImage(isNeutral(adjustment) ? cropped : await adjustImage(cropped, adjustment))
        setPreviewing(true)
    })

    const handleBackToEdit = () => {
        replaceFinalImage(null)
        setPreviewing(false)
    }

    const handleUpload = async (blob: Blob) => {
        const index = currentImageIndex
        if (index === null || !imageStatuses[index]) return

        const imageStatus = imageStatuses[index]
        const baseName = (imageStatus.file.name || 'foto').replace(/\.[^.]+$/, '')
        const croppedFile = new File([blob], `${baseName}.jpg`, { type: 'image/jpeg', lastModified: Date.now() })

        if (!await onUpload(croppedFile)) return

        setImageStatuses(prev =>
            prev.map((status, i) => (i === index ? { ...status, status: 'uploaded', croppedFile } : status))
        )
        if (imageStatuses.every((status, i) => i === index || status.status === 'uploaded')) {
            closeDialog()
            return
        }
        resetCropperState()
        const nextPendingIndex = imageStatuses.findIndex((status, i) => i !== index && status.status === 'pending')
        if (nextPendingIndex !== -1) {
            selectImageForCropping(nextPendingIndex)
        } else {
            setCurrentImageIndex(null)
        }
    }

    // Picked photos open straight in the editor, so the first pending one gets selected.
    useEffect(() => {
        if (imageStatuses.length === 0) return
        if (currentImageIndex !== null && imageStatuses[currentImageIndex]?.status === 'cropping') return

        const firstPendingIdx = imageStatuses.findIndex(status => status.status === 'pending')
        if (firstPendingIdx !== -1) {
            // selection follows async status changes from several handlers, so it stays an effect
            // eslint-disable-next-line react-hooks/set-state-in-effect
            selectImageForCropping(firstPendingIdx)
        } else if (currentImageIndex !== null) {
            setCurrentImageIndex(null)
        }
    }, [imageStatuses, currentImageIndex, selectImageForCropping])

    const epaper = epaperView === 'epaper'
    const showsEpaperToggle = previewing || tool !== 'crop'
    const title = imageStatuses.length > 1 && currentImageIndex !== null
        ? `Foto ${currentImageIndex + 1} von ${imageStatuses.length}`
        : dialogTitle

    return (
        <AppDialog
            fullScreen={!matches}
            open={open}
            onClose={handleDialogClose}
            aria-labelledby="upload-dialog-title"
            maxWidth="lg"
            fullWidth
            slots={{
                transition: matches ? ZoomTransition : SlideTransition
            }}
            slotProps={{
                paper: { sx: { height: { md: 'min(900px, calc(100% - 64px))' } } },
                transition: { onEntered: () => setEntered(true), onExited: () => setEntered(false) }
            }}
        >
            <input {...inputProps} />
            <EditorLayout
                title={title}
                subtitle={currentImage?.file.name}
                onClose={handleDialogClose}
                onBack={previewing ? handleBackToEdit : undefined}
                actions={currentImage && showsEpaperToggle && <EpaperToggle value={epaperView} onChange={setEpaperView} />}
                strip={imageStatuses.length > 0 && (
                    <>
                        <ThumbStrip
                            imageStatuses={imageStatuses}
                            currentImageIndex={currentImageIndex}
                            imagePreviews={imagePreviews}
                            previewErrors={previewErrors}
                            markPreviewBroken={markPreviewBroken}
                            onSelect={handleSelectPhoto}
                            onRemove={index => setRemoveId(imageStatuses[index].id)}
                            onAdd={() => openPicker('gallery')}
                            canAdd={imageStatuses.length < maxFiles}
                            isPreparing={isPreparing}
                        />
                        <LinearProgress sx={{ visibility: busy ? 'visible' : 'hidden' }} />
                        <Box role="status" sx={visuallyHidden}>
                            {uploading ? 'Wird hochgeladen…' : busy ? 'Foto wird vorbereitet…' : ''}
                        </Box>
                    </>
                )}
                footer={currentImage && (previewing && finalImage ? (
                    <Box sx={{ display: 'flex', gap: 2 }}>
                        {matches && (
                            <Button variant="outlined" size="large" fullWidth onClick={handleBackToEdit} disabled={uploading}>
                                Zurück zum Bearbeiten
                            </Button>
                        )}
                        <Button
                            variant="contained"
                            size="large"
                            fullWidth
                            disabled={uploading}
                            startIcon={uploading ? <CircularProgress size={18} color="inherit" /> : <CloudUploadIcon />}
                            onClick={() => handleUpload(finalImage.blob)}
                        >
                            Hochladen
                        </Button>
                    </Box>
                ) : (
                    <Button
                        variant="contained"
                        size="large"
                        fullWidth
                        disabled={editLocked || uploading || !!previewErrors[currentImage.id]}
                        startIcon={busy ? <CircularProgress size={18} color="inherit" /> : undefined}
                        onClick={goToPreview}
                    >
                        Weiter zur Vorschau
                    </Button>
                ))}
            >
                {imageStatuses.length === 0 && (
                    <StartScreen
                        openPicker={openPicker}
                        selectFiles={selectFiles}
                        isPreparing={isPreparing}
                        showCamera={showCamera}
                        maxFiles={maxFiles}
                    />
                )}
                {currentImage && !previewing && entered && (
                    <EditorGrid>
                        {tool === 'crop' && (
                            <ImageCropping
                                currentImage={currentImage}
                                preview={imagePreviews[currentImage.id]}
                                previewError={previewErrors[currentImage.id]}
                                markPreviewBroken={markPreviewBroken}
                                setCroppedAreaPixels={setCroppedAreaPixels}
                                crop={crop}
                                setCrop={setCrop}
                                zoom={zoom}
                                setZoom={setZoom}
                                rotation={rotation}
                                setRotation={setRotation}
                                fillColor={fillColor}
                                setFillColor={setFillColor}
                            />
                        )}
                        {tool === 'adjust' && croppedImage && (
                            <ImageAdjust
                                epaper={epaper}
                                croppedImage={croppedImage}
                                fileName={currentImage.file.name}
                                adjustment={adjustment}
                                setAdjustment={setAdjustment}
                            />
                        )}
                        <ToolTabs value={tool} onChange={changeTool} disabled={editLocked} />
                    </EditorGrid>
                )}
                {previewing && finalImage && currentImage && (
                    <ImagePreview
                        finalImage={finalImage}
                        epaper={epaper}
                        fileName={currentImage.file.name}
                    />
                )}
            </EditorLayout>
            <ConfirmDialog
                open={confirmDiscard}
                title="Bearbeitung verwerfen?"
                confirmLabel="Verwerfen"
                cancelLabel="Weiter bearbeiten"
                destructive
                backToClose={false}
                onConfirm={closeDialog}
                onCancel={() => setConfirmDiscard(false)}
            >
                Noch nicht hochgeladene Fotos und ihre Bearbeitung gehen verloren.
            </ConfirmDialog>
            <ConfirmDialog
                open={removeIndex !== -1}
                title="Foto entfernen?"
                confirmLabel="Entfernen"
                cancelLabel="Behalten"
                destructive
                onConfirm={confirmRemove}
                onCancel={() => setRemoveId(null)}
            >
                {`${imageStatuses[removeIndex]?.file.name ?? ''} und seine Bearbeitung werden aus der Auswahl entfernt.`}
            </ConfirmDialog>
        </AppDialog>
    );
}

export default PictureUploadDialog
