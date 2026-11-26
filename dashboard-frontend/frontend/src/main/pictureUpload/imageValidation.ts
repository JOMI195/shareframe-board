const MIME_TYPES: Record<string, string> = {
    jpg: 'image/jpeg',
    jpeg: 'image/jpeg',
    png: 'image/png',
    webp: 'image/webp',
    heic: 'image/heic',
    heif: 'image/heif',
};

// The editor re-encodes every photo, so the input only has to be readable by the browser.
export const MAX_INPUT_SIZE_MB = 30;

export const getAcceptedFileTypes = (): string => Array.from(new Set(Object.values(MIME_TYPES))).join(',');

export const getAllowedExtensionLabels = (): string[] => Object.keys(MIME_TYPES);

// Some mobile pickers hand over files without a usable name, so the MIME type counts too.
const hasValidFormat = (file: File): boolean => {
    const name = file.name.toLowerCase();
    return Object.keys(MIME_TYPES).some(extension => name.endsWith(`.${extension}`))
        || Object.values(MIME_TYPES).includes(file.type);
};

export const validateImage = (file: File): string[] => {
    const errors: string[] = [];
    if (!hasValidFormat(file)) errors.push('Falsches Dateiformat');
    if (file.size > MAX_INPUT_SIZE_MB * 1024 * 1024) errors.push(`Datei ist zu groß (maximal ${MAX_INPUT_SIZE_MB} MB)`);
    return errors;
};

export const validateImages = (files: File[], freeSlots: number) => {
    const validFiles: File[] = [];
    const invalidFiles: { file: File, errors: string[] }[] = [];
    files.forEach((file) => {
        const errors = validateImage(file);
        if (errors.length === 0 && validFiles.length >= freeSlots) {
            errors.push(`Es passen nur noch ${freeSlots} Fotos dazu`);
        }
        if (errors.length === 0) validFiles.push(file);
        else invalidFiles.push({ file, errors });
    });
    return { validFiles, invalidFiles };
};
