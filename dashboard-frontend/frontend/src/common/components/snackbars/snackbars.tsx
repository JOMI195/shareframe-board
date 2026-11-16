import { useEffect } from "react";
import { Outlet } from "react-router";
import AlertSnackbar from "./alertSnackbar";
import LoadingSnackbar from "./loadingSnackbar";
import { useAppDispatch, useAppSelector } from "@/store";
import { getSnackbars, removeAlertSnackbar, removeAllLoadingSnackbars } from '@/store/snackbars/snackbars.Slice';

export const Snackbars = () => {
    const dispatch = useAppDispatch();
    const { alerts, loading } = useAppSelector(getSnackbars);

    useEffect(() => {
        dispatch(removeAllLoadingSnackbars());
    }, [dispatch]);

    return (
        <>
            <Outlet />
            {alerts.map((snackbar) => (
                <AlertSnackbar
                    key={snackbar.id}
                    snackbar={snackbar}
                    removeSnackbar={() => dispatch(removeAlertSnackbar(snackbar.id))}
                />
            ))}
            {loading.map((snackbar) => (
                <LoadingSnackbar key={snackbar.id} snackbar={snackbar} />
            ))}
        </>
    );
};

export default Snackbars;
