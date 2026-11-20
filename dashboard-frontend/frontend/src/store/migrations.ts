import { MigrationManifest } from 'redux-persist';

// The stored dialogs would shadow keys added since (autoMergeLevel2 replaces
// each slice wholesale), so drop them and let the slice re-initialise.
const dropDialogs = (state: Record<string, unknown> | undefined) => {
    if (!state) return state;
    const { dialogs, ...rest } = state;
    return rest;
};

// Only these survive a reload; everything else is the board's state and asked again
// (a stored spinner or error would otherwise come back on every load).
export const PERSISTED = ['navigation', 'loadingWall'] as const;

const keepPersisted = (state: Record<string, unknown> | undefined) => {
    if (!state) return state;
    return Object.fromEntries(Object.entries(state).filter(([key]) =>
        key === '_persist' || (PERSISTED as readonly string[]).includes(key)));
};

const typedMigrations = {
    3: dropDialogs,
    4: dropDialogs,
    5: keepPersisted,
};

const migrations = typedMigrations as unknown as MigrationManifest;

export default migrations;
