import { RootState } from "@/store";
import { createSlice } from "@reduxjs/toolkit";

type SliceState = {
    general: {
        shutdown: { open: boolean };
        restart: { open: boolean };
        changePassword: { open: boolean };
    },
    updates: {
        confirmUpdate: { open: boolean };
    },
    network: {
        addNetwork: { open: boolean };
        forgetNetwork: { open: boolean; ssid: string };
        renameNetwork: { open: boolean; ssid: string; newName: string };
        changeApPassword: { open: boolean };
    }
};

const initialState: SliceState = {
    general: {
        shutdown: { open: false },
        restart: { open: false },
        changePassword: { open: false }
    },
    updates: {
        confirmUpdate: { open: false }
    },
    network: {
        addNetwork: { open: false },
        forgetNetwork: { open: false, ssid: '' },
        renameNetwork: { open: false, ssid: '', newName: '' },
        changeApPassword: { open: false }
    }
};

const dialogsSlice = createSlice({
    name: "dialogs",
    initialState,
    reducers: {
        shutdownDialogOpened: (state) => {
            state.general.shutdown.open = true;
        },
        shutdownDialogClosed: (state) => {
            state.general.shutdown.open = false;
        },
        restartDialogOpened: (state) => {
            state.general.restart.open = true;
        },
        restartDialogClosed: (state) => {
            state.general.restart.open = false;
        },
        updatesConfirmUpdateDialogOpened: (state) => {
            state.updates.confirmUpdate.open = true;
        },
        updatesConfirmUpdateDialogClosed: (state) => {
            state.updates.confirmUpdate.open = false;
        },
        networkAddNetworkDialogOpened: (state) => {
            state.network.addNetwork.open = true;
        },
        networkAddNetworkDialogClosed: (state) => {
            state.network.addNetwork.open = false;
        },
        networkForgetNetworkDialogOpened: (state, action) => {
            state.network.forgetNetwork.open = true;
            state.network.forgetNetwork.ssid = action.payload.ssid;
        },
        networkForgetNetworkDialogClosed: (state) => {
            state.network.forgetNetwork.open = false;
            state.network.forgetNetwork.ssid = '';
        },
        networkRenameNetworkDialogOpened: (state, action) => {
            state.network.renameNetwork.open = true;
            state.network.renameNetwork.ssid = action.payload.ssid;
            state.network.renameNetwork.newName = action.payload.ssid;
        },
        networkRenameNetworkDialogClosed: (state) => {
            state.network.renameNetwork.open = false;
            state.network.renameNetwork.ssid = '';
            state.network.renameNetwork.newName = '';
        },
        networkRenameNetworkNameChanged: (state, action) => {
            state.network.renameNetwork.newName = action.payload.newName;
        },
        generalChangePasswordDialogOpened: (state) => {
            state.general.changePassword.open = true;
        },
        generalChangePasswordDialogClosed: (state) => {
            state.general.changePassword.open = false;
        },
        networkChangeApPasswordDialogOpened: (state) => {
            state.network.changeApPassword.open = true;
        },
        networkChangeApPasswordDialogClosed: (state) => {
            state.network.changeApPassword.open = false;
        },
    },
});

export const openShutdownDialog = () => ({
    type: dialogsSlice.actions.shutdownDialogOpened.type,
});

export const closeShutdownDialog = () => ({
    type: dialogsSlice.actions.shutdownDialogClosed.type,
});

export const openRestartDialog = () => ({
    type: dialogsSlice.actions.restartDialogOpened.type,
});

export const closeRestartDialog = () => ({
    type: dialogsSlice.actions.restartDialogClosed.type,
});

export const openUpdatesConfirmUpdateDialog = () => ({
    type: dialogsSlice.actions.updatesConfirmUpdateDialogOpened.type,
});

export const closeUpdatesConfirmUpdateDialog = () => ({
    type: dialogsSlice.actions.updatesConfirmUpdateDialogClosed.type,
});

export const openNetworkAddNetworkDialog = () => ({
    type: dialogsSlice.actions.networkAddNetworkDialogOpened.type,
});

export const closeNetworkAddNetworkDialog = () => ({
    type: dialogsSlice.actions.networkAddNetworkDialogClosed.type,
});

export const openNetworkForgetNetworkDialog = (ssid: string) => ({
    type: dialogsSlice.actions.networkForgetNetworkDialogOpened.type,
    payload: { ssid },
});

export const closeNetworkForgetNetworkDialog = () => ({
    type: dialogsSlice.actions.networkForgetNetworkDialogClosed.type,
});

export const openNetworkRenameNetworkDialog = (ssid: string) => ({
    type: dialogsSlice.actions.networkRenameNetworkDialogOpened.type,
    payload: { ssid },
});

export const closeNetworkRenameNetworkDialog = () => ({
    type: dialogsSlice.actions.networkRenameNetworkDialogClosed.type,
});

export const updateNetworkRenameNetworkName = (newName: string) => ({
    type: dialogsSlice.actions.networkRenameNetworkNameChanged.type,
    payload: { newName },
});

export const openGeneralChangePasswordDialog = () => ({
    type: dialogsSlice.actions.generalChangePasswordDialogOpened.type,
});

export const closeGeneralChangePasswordDialog = () => ({
    type: dialogsSlice.actions.generalChangePasswordDialogClosed.type,
});

export const openNetworkChangeApPasswordDialog = () => ({
    type: dialogsSlice.actions.networkChangeApPasswordDialogOpened.type,
});

export const closeNetworkChangeApPasswordDialog = () => ({
    type: dialogsSlice.actions.networkChangeApPasswordDialogClosed.type,
});

export const {
    shutdownDialogOpened,
    shutdownDialogClosed,
    restartDialogOpened,
    restartDialogClosed,
    updatesConfirmUpdateDialogOpened,
    updatesConfirmUpdateDialogClosed,
    networkAddNetworkDialogOpened,
    networkAddNetworkDialogClosed,
    networkForgetNetworkDialogOpened,
    networkForgetNetworkDialogClosed,
    networkRenameNetworkDialogOpened,
    networkRenameNetworkDialogClosed,
    networkRenameNetworkNameChanged,
    generalChangePasswordDialogOpened,
    generalChangePasswordDialogClosed,
    networkChangeApPasswordDialogOpened,
    networkChangeApPasswordDialogClosed
} = dialogsSlice.actions;

export default dialogsSlice.reducer;

export const getDialogs = (state: RootState) => state.dialogs;