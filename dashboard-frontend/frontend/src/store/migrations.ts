import { MigrationManifest } from 'redux-persist';

// The stored dialogs would shadow keys added since (autoMergeLevel2 replaces
// each slice wholesale), so drop them and let the slice re-initialise.
const dropDialogs = (state: Record<string, unknown> | undefined) => {
    if (!state) return state;
    const { dialogs, ...rest } = state;
    return rest;
};

const typedMigrations = {
    3: dropDialogs,
    4: dropDialogs,
};

const migrations = typedMigrations as unknown as MigrationManifest;

export default migrations;
