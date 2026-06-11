import { MigrationManifest } from 'redux-persist';

const typedMigrations = {
    // v3: dialogs gained changePassword/changeApPassword. Drop the persisted
    // copy (autoMergeLevel2 would otherwise shadow the new keys) so the slice
    // re-initialises with the full shape.
    3: (state: any) => {
        if (!state) return state;
        const { dialogs, ...rest } = state;
        return rest;
    },
};

const migrations = typedMigrations as unknown as MigrationManifest;

export default migrations;