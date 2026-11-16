import { CircularProgress, Paper, Slide, Typography } from "@mui/material";
import { StackedSnackbar } from "./snackbarStack";
import { useColorThemeContext } from "@/context/colorTheme/colorThemeContextValue";
import { SnackbarItem } from "@/store/snackbars/snackbars.Slice";
import { useDelayedLoading } from "@/hooks/loading/useDelayedLoading";

interface ILoadingSnackbarProps {
    snackbar: SnackbarItem;
}

// Only shows once the action takes a moment, so fast requests don't flash a snackbar.
const LoadingSnackbar = ({ snackbar }: ILoadingSnackbarProps) => {
    const { colorMode } = useColorThemeContext();
    const open = useDelayedLoading(true);

    return (
        <StackedSnackbar
            open={open}
            slots={{ transition: Slide }}
            slotProps={{ clickAwayListener: { onClickAway: () => null } }}
        >
            <Paper
                role="status"
                elevation={5}
                sx={{
                    py: 1.5,
                    px: 3,
                    display: 'flex',
                    alignItems: "center",
                    justifyContent: "space-between",
                    flexDirection: "row",
                    background: (theme) => colorMode === "light" ? theme.palette.common.black : theme.palette.common.white,
                    color: (theme) => colorMode === "light" ? theme.palette.common.white : theme.palette.common.black,
                }}
            >
                <CircularProgress color="inherit" size="1.1rem" />
                <Typography variant="subtitle2" sx={{ ml: 5 }}>{snackbar.message}</Typography>
            </Paper>
        </StackedSnackbar>
    );
};

export default LoadingSnackbar;
