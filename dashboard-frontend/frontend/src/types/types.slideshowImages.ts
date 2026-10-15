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
}
