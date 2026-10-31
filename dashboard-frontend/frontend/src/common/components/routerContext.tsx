import { useAppDispatch } from "@/store";
import { setCurrentPath } from "@/store/navigation/navigation.Slice";
import { useEffect } from "react";
import { Outlet, useLocation } from "react-router";
import { fetchConnectionMode } from "@/store/connectionMode/connectionMode.Slice";

const RouterContext = () => {
    const dispatch = useAppDispatch();
    const location = useLocation();

    useEffect(() => {
        dispatch(setCurrentPath(location.pathname));
    }, [dispatch, location.pathname]);

    // Central network-mode poll; the banner, header and sign-in read the slice.
    useEffect(() => {
        dispatch(fetchConnectionMode());
        const id = setInterval(() => dispatch(fetchConnectionMode()), 10000);
        return () => clearInterval(id);
    }, [dispatch]);

    return (
        <Outlet />
    )
}

export default RouterContext;
