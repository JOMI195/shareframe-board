import { RootState } from "@/store";
import { createSlice, PayloadAction } from "@reduxjs/toolkit";

type SliceState = {
  sidebar: { open: boolean };
  currentPath: string;
};

const initialState: SliceState = {
  sidebar: { open: false },
  currentPath: '',
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

export const {
  sidebarOpened,
  sidebarClosed,
  currentPathSet
} = navigationSlice.actions;

export default navigationSlice.reducer;

export const getSidebar = (state: RootState) =>
  state.navigation.sidebar;

export const getCurrentPath = (state: RootState) =>
  state.navigation.currentPath;