// Android Chrome stages picker files in a temp location it may release once the
// input is touched again; a copy detaches us from that lifetime.
export const materializeFile = async (file: File): Promise<File> => {
    const bytes = await file.arrayBuffer();
    return new File([bytes], file.name, { type: file.type, lastModified: file.lastModified });
}

export const materializeFiles = async (
    files: File[]
): Promise<{ copies: File[], failures: { file: File, reason: string }[] }> => {
    const copies: File[] = [];
    const failures: { file: File, reason: string }[] = [];

    // Sequential: one buffer in flight instead of all of them.
    for (const file of files) {
        try {
            copies.push(await materializeFile(file));
        } catch (error) {
            failures.push({
                file,
                reason: error instanceof Error ? (error.name || error.message) : 'Unbekannter Fehler'
            });
        }
    }

    return { copies, failures };
}
