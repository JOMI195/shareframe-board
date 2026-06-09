import { RootState } from "@/store";
import { createSlice, PayloadAction } from "@reduxjs/toolkit";

type SliceState = {
  sidebar: { open: boolean };
  currentPath: string;
  advancedMode: boolean;
};

const initialState: SliceState = {
  sidebar: { open: false },
  currentPath: '',
  advancedMode: false,
};

const navigationSlice = createSlice({
  name: "navigation",
  initialState,
  reducers: {
    sidebarOpened: (state) => {
      state.sidebar.open = true;
    },
    sidebarClosed: (state) => {
      state.sidebar.open = false;
    },
    currentPathSet: (state, action: PayloadAction<string>) => {
      state.currentPath = action.payload;
    },
    advancedModeToggled: (state) => {
      state.advancedMode = !state.advancedMode;
    },
  },
});

export const openSidedbar = () => ({
  type: sidebarOpened.type,
});

export const closeSidebar = () => ({
  type: sidebarClosed.type,
});

export const setCurrentPath = (path: string) => ({
  type: currentPathSet.type,
  payload: path,
});

export const toggleAdvancedMode = () => ({
  type: advancedModeToggled.type,
});

export const {
  sidebarOpened,
  sidebarClosed,
  currentPathSet,
  advancedModeToggled
} = navigationSlice.actions;

export default navigationSlice.reducer;

export const getSidebar = (state: RootState) =>
  state.navigation.sidebar;

export const getCurrentPath = (state: RootState) =>
  state.navigation.currentPath;

export const getAdvancedMode = (state: RootState) =>
  state.navigation.advancedMode;