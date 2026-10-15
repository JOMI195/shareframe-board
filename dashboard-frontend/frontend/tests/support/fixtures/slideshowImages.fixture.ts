// Wire shape of GET /api/frame/images `data`, not the slice state.
export interface SlideshowImagePayload {
  id: number;
  sender: string;
  received_at: number;
  expires_at: number;
  width: number;
  height: number;
  size_bytes: number;
  position: number;
}

export interface SlideshowImagesPayload {
  mode: 'shuffle' | 'chronological' | 'newest_first' | 'custom';
  custom_order: number[];
  current_image_id: number;
  images: SlideshowImagePayload[];
}

export const makeSlideshowImage = (over: Partial<SlideshowImagePayload> = {}): SlideshowImagePayload => ({
  id: 1,
  sender: 'alice',
  received_at: 1_760_000_000,
  expires_at: 1_760_086_400,
  width: 1600,
  height: 960,
  size_bytes: 345_678,
  position: 1,
  ...over,
});

/** Three pictures from different senders in chronological order. */
export const makeSlideshowImages = (over: Partial<SlideshowImagesPayload> = {}): SlideshowImagesPayload => ({
  mode: 'chronological',
  custom_order: [],
  current_image_id: 0,
  images: [
    makeSlideshowImage({ id: 11, sender: 'alice', received_at: 1_760_000_000, position: 1 }),
    makeSlideshowImage({ id: 12, sender: 'bob', received_at: 1_760_000_600, width: 800, height: 480, position: 2 }),
    makeSlideshowImage({ id: 13, sender: 'carol', received_at: 1_760_001_200, position: 3 }),
  ],
  ...over,
});
