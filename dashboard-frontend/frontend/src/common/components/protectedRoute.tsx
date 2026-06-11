import { useAppSelector } from "@/store";
import { selectAuth } from "@/store/auth/auth.Slice";
import { selectConnectionMode } from "@/store/connectionMode/connectionMode.Slice";
import { Navigate, Outlet } from "react-router";
import { getAuthenticationUrl, getSignInUrl } from "@/assets/endpoints/app/authEndpoints";
import { getSetupUrl } from "@/assets/endpoints/app/appEndpoints";

interface ProtectedRouteProps {
  redirectPath?: string;
}

const ProtectedRoute = ({
  redirectPath = getAuthenticationUrl() + getSignInUrl(),
}: ProtectedRouteProps) => {
  const { isAuthenticated } = useAppSelector(selectAuth);
  const { mode, loaded } = useAppSelector(selectConnectionMode);

  if (!isAuthenticated) {
    // AP fallback keeps the setup-first flow: land on the WiFi-setup page,
    // which links to the (offline-capable) password sign-in.
    const target = loaded && mode === 'ap' ? getSetupUrl() : redirectPath;
    return <Navigate to={target} replace />;
  }

  return <Outlet />;
};

export default ProtectedRoute;
