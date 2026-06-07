export const isVersionNewer = (latest: string, current: string): boolean => {
    if (!latest || !current) return false;
    const [lMajor, lMinor, lPatch] = latest.split('.').map(Number);
    const [cMajor, cMinor, cPatch] = current.split('.').map(Number);

    return lMajor > cMajor || (lMajor === cMajor && lMinor > cMinor) || (lMajor === cMajor && lMinor === cMinor && lPatch > cPatch);
};
