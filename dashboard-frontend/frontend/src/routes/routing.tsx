import MainLayout from '@/common/components/layout/layout';
import Snackbars from '@/common/components/snackbars/snackbars';
import ProtectedRoute from '@/common/components/protectedRoute';
import { getHomeUrl, getGeneralSettingsUrl, getLogsUrl, getNetworkUrl, getUpdatesyUrl } from '@/assets/endpoints/app/appEndpoints';
import FrameActions from '@/main/frame/frameActions';
import Updates from '@/main/updates/updates';
import Network from '@/main/network/network';
import General from '@/main/general/general';
import FrameLogs from '@/main/monitoring/frameLogs';
import NotFound from '@/common/components/notFound';
import { createBrowserRouter } from 'react-router';
import { getAuthenticationUrl, getSignInUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import AuthenticationLayout from '@/main/authentication/layout';
import SignIn from '@/main/authentication/signIn/signIn';
import SignOut from '@/main/authentication/signOut/signOut';
import RouterContext from '@/common/components/routerContext';

const Routing = createBrowserRouter([
  {
    element: <RouterContext />,
    children: [
      {
        element: <Snackbars />,
        children: [
          {
            path: "*",
            element: <NotFound />,
          },
          {
            path: getAuthenticationUrl(),
            element: <AuthenticationLayout />,
            children: [
              {
                path: "*",
                element: <NotFound />,
              },
              {
                path: getAuthenticationUrl() + getSignInUrl(),
                element: <SignIn />,
              },
              {
                path: getAuthenticationUrl() + getSignOutUrl(),
                element: <SignOut />,
              },
            ],
          },
          {
            element: <ProtectedRoute />,
            children: [
              {
                element: <MainLayout />,
                children: [
                  {
                    path: "*",
                    element: <NotFound />,
                  },
                  {
                    path: getHomeUrl(),
                    element: <FrameActions />
                  },
                  {
                    path: getNetworkUrl(),
                    element: <Network />
                  },
                  {
                    path: getUpdatesyUrl(),
                    element: <Updates />
                  },
                  {
                    path: getGeneralSettingsUrl(),
                    element: <General />
                  },
                  {
                    path: getLogsUrl(),
                    element: <FrameLogs />
                  },
                ],
              },
            ]
          },
        ],
      }
    ]
  },
]);

export default Routing;
