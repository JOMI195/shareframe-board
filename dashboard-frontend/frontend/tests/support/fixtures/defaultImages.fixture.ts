// Wire shape of GET /api/frame/default-images `data`, not the slice state.
export interface DefaultImagePayload {
  name: string;
  width: number;
  height: number;
  size_bytes: number;
}

export interface DefaultImagesPayload {
  min: number;
  max: number;
  images: DefaultImagePayload[];
}

export const makeDefaultImage = (index: number): DefaultImagePayload => ({
  name: `${String(index).repeat(64).slice(0, 64)}.jpg`,
  width: 800,
  height: 480,
  size_bytes: 120_000 + index,
});

export const makeDefaultImages = (count = 3): DefaultImagesPayload => ({
  min: 3,
  max: 10,
  images: Array.from({ length: count }, (_, i) => makeDefaultImage(i + 1)),
});
