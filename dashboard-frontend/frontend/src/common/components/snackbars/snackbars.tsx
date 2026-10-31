import { Box } from "@mui/material";
import ViewerAlertSnackbar from "../snackbars/alertSnackbar";
import ViewerLoadingSnackbar from "../snackbars/loadingSnackbar";
import { useAppSelector, useAppDispatch } from "@/store";
import { AlertColor } from '@mui/material';
import { getSnackbars, removeAlertSnackbar, removeAllLoadingSnackbars } from '@/store/snackbars/snackbars.Slice';
import { Outlet } from "react-router";
import { useEffect } from "react";

interface SnackbarItem {
    id: string;
    message: string;
}

interface AlertSnackbarItem extends SnackbarItem {
    severity: AlertColor;
}

function isAlertSnackbar(snackbar: SnackbarItem | AlertSnackbarItem): snackbar is AlertSnackbarItem {
    return 'severity' in snackbar;
}

export const Snackbars = () => {
    const dispatch = useAppDispatch();
    const snackbars = useAppSelector(getSnackbars);

    const handleCloseAlert = (id: string) => {
        dispatch(removeAlertSnackbar(id));
    };

    const allSnackbars = [...snackbars.alerts, ...snackbars.loading]
        .sort((a, b) => b.id.localeCompare(a.id));

    useEffect(() => {
        dispatch(removeAllLoadingSnackbars());
    }, [dispatch]);

    return (
        <Box>
            <Outlet />
            <Box
                sx={(theme) => ({
                    position: 'fixed',
                    // Above the page's floating action buttons on phones.
                    bottom: {
                        xs: 'calc(88px + env(safe-area-inset-bottom))',
                        sm: 'calc(16px + env(safe-area-inset-bottom))',
                    },
                    left: 0,
                    right: 0,
                    px: 1,
                    zIndex: theme.zIndex.snackbar,
                    display: 'flex',
                    flexDirection: 'column',
                    alignItems: 'center',
                    gap: 1,
                    pointerEvents: 'none',
                    '& > *': { pointerEvents: 'auto', maxWidth: '100%' },
                })}
            >
                {allSnackbars.map((snackbar) => isAlertSnackbar(snackbar) ? (
                    <ViewerAlertSnackbar
                        key={snackbar.id}
                        snackbar={snackbar}
                        closeSnackbar={() => handleCloseAlert(snackbar.id)}
                    />
                ) : (
                    <ViewerLoadingSnackbar key={snackbar.id} snackbar={snackbar} />
                ))}
            </Box>
        </Box>
    );
};

export default Snackbars;