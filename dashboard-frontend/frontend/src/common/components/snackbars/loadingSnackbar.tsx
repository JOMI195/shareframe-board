import { CircularProgress, Paper, Theme, Typography, Box } from "@mui/material";
import { useColorThemeContext } from "@/context/colorTheme/colorThemeContext";
import { SnackbarItem } from "@/store/snackbars/snackbars.Slice";

interface ILoadingSnackbarProps {
    snackbar: SnackbarItem;
    closeSnackbar: () => void;
}

const LoadingSnackbar = ({ snackbar }: ILoadingSnackbarProps) => {
    const { colorMode } = useColorThemeContext();

    return (
        <Box
            sx={{
                display: 'flex',
                justifyContent: 'center',
                width: '100%',
                position: 'relative', // Allow stacking based on parent position
            }}
        >
            <Paper
                elevation={5}
                sx={{
                    py: 2.5,
                    px: 3,
                    display: 'flex',
                    alignItems: "center",
                    justifyContent: "space-between",
                    flexDirection: "row",
                    background: (theme: Theme) => colorMode === "light" ? theme.palette.common.black : theme.palette.common.white,
                    color: (theme: Theme) => colorMode === "light" ? theme.palette.common.white : theme.palette.common.black,
                    cursor: 'default', // To allow clicking and closing
                    minWidth: '300px', // Ensuring a minimum width
                    maxWidth: '500px', // Optional: limit the max width
                }}
            >
                <CircularProgress
                    color="inherit"
                    size={25}
                />
                <Typography variant="subtitle2" sx={{ ml: 5 }}>
                    {snackbar.message}
                </Typography>
            </Paper>
        </Box>
    );
};

export default LoadingSnackbar;