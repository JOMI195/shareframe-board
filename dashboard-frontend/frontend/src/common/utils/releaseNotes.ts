export type ReleaseNotesBlock =
    | { type: 'paragraph'; text: string }
    | { type: 'list'; items: string[] };

const bulletPattern = /^[-*]\s+/;

export const parseReleaseNotes = (notes: string): ReleaseNotesBlock[] => {
    const blocks: ReleaseNotesBlock[] = [];
    for (const rawLine of notes.split(/\r?\n/)) {
        const line = rawLine.trim();
        if (!line) continue;
        if (bulletPattern.test(line)) {
            const item = line.replace(bulletPattern, '');
            const last = blocks[blocks.length - 1];
            if (last?.type === 'list') last.items.push(item);
            else blocks.push({ type: 'list', items: [item] });
        } else {
            blocks.push({ type: 'paragraph', text: line });
        }
    }
    return blocks;
};
