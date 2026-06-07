import {
    Box,
    Typography,
    Stack,
    CircularProgress,
} from '@mui/material';
import { useAppSelector } from '@/store';
import { selectFrameInfoState } from '@/store/frameInfo/frameInfo.Slice';
import { selectUpdatesState } from '@/store/updates/updates.Slice';
import { isVersionNewer } from '@/common/utils/version';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import UpdateIcon from '@mui/icons-material/Update';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';
import Dialogs from './dialogs/dialogs';
import { Actions } from './actions/actions';

const Updates = () => {
    const { latest_release, loading } = useAppSelector(selectUpdatesState);
    const { frameInfo } = useAppSelector(selectFrameInfoState);

    const isNewVersion = (latest_release && frameInfo && isVersionNewer(latest_release.version, frameInfo.version)) ?? false;

    return (
        <>
            <Stack width={"100%"} spacing={3}>
                <ShareframeInfoCard
                    title="Übersicht"
                    sections={[
                        {
                            label: "Aktuelle Version",
                            content: frameInfo?.version
                        },
                    ]}
                />

                {
                    loading ? (
                        <ShareframeInfoCard
                            sections={[
                                {
                                    content: {
                                        type: 'reactNode',
                                        value: (
                                            <Box sx={{ display: 'flex', alignItems: 'center' }}>
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <CircularProgress size={21} sx={{ mr: 1 }} />Suche nach Updates
                                                </Typography>
                                            </Box>
                                        )
                                    },
                                }
                            ]}
                        />
                    ) : (
                        isNewVersion ? (
                            <ShareframeInfoCard
                                sections={[
                                    {
                                        content: {
                                            type: 'reactNode',
                                            value: (
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <UpdateIcon color='error' sx={{ mr: 1 }} />Neue Version verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        label: "Neue Version",
                                        content: latest_release?.version
                                    },
                                    {
                                        label: "Umfang",
                                        content: latest_release?.release_notes
                                    }
                                ]}
                            />
                        ) : (
                            <ShareframeInfoCard
                                sections={[
                                    {
                                        content: {
                                            type: 'reactNode',
                                            value: (
                                                <Typography variant="h6" color="text.secondary" gutterBottom sx={{ display: "flex", alignItems: "center" }}>
                                                    <CheckCircleIcon color='success' sx={{ mr: 1 }} />Keine Updates verfügbar
                                                </Typography>
                                            )
                                        }
                                    },
                                    {
                                        content: "Bilderrahmen ist auf dem neusten Stand!"
                                    },
                                ]}
                            />

                        )
                    )
                }
            </Stack >

            <Actions />
            <Dialogs />
        </>
    );
};

export default Updates;