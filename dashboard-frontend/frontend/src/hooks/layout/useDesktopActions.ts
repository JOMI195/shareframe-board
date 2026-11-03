import { useMediaQuery, useTheme } from "@mui/material";

// Below this breakpoint page actions float at the bottom instead.
export const useDesktopActions = () => {
    const theme = useTheme();
    return useMediaQuery(theme.breakpoints.up("md"));
};
