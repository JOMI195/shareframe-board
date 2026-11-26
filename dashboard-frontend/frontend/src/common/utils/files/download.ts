import { fetchWithTimeout, SLOW_TIMEOUT_MS } from '@/common/utils/fetch';

/** Saves a board file through the browser; throws when the board does not hand it over. */
export const downloadBoardFile = async (url: string, fileName: string) => {
    const response = await fetchWithTimeout(url, {}, SLOW_TIMEOUT_MS);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const objectUrl = URL.createObjectURL(await response.blob());
    const link = document.createElement('a');
    link.href = objectUrl;
    link.download = fileName;
    link.click();
    URL.revokeObjectURL(objectUrl);
};
