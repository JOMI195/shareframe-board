export type PlaybackMode = 'shuffle' | 'chronological' | 'newest_first' | 'custom';

export interface ISlideshowImage {
    id: number;
    sender: string;
    receivedAt: number | null; // unix seconds; null when the board stored it before tracking arrival
    expiresAt: number; // unix seconds
    width: number;
    height: number;
    sizeBytes: number;
    position: number;
}

export interface INowShowing {
    seq: number;
    state: 'empty' | 'image' | 'cleared';
    imageId: number | null;
    sender: string | null;
    /** What the picture on the panel is; null unless state is 'image'. */
    kind: 'sent' | 'default' | 'system' | null;
    /** Name of the default image shown, when it can be fetched from the board. */
    defaultImage: string | null;
}
