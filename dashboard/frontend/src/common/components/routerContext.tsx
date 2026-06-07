import { useAppDispatch } from "@/store";
import { setCurrentPath } from "@/store/navigation/navigation.Slice";
import { useEffect } from "react";
import { Outlet, useLocation } from "react-router";

const RouterContext = () => {
    const dispatch = useAppDispatch();
    const location = useLocation();

    useEffect(() => {
        dispatch(setCurrentPath(location.pathname));

    }, [dispatch, location.pathname]);

    return (
        <Outlet />
    )
}

export default RouterContext;