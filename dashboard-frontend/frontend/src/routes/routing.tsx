import MainLayout from '@/common/components/layout/layout';
import Snackbars from '@/common/components/snackbars/snackbars';
import ProtectedRoute from '@/common/components/protectedRoute';
import { getHomeUrl, getGeneralSettingsUrl, getDisplayHealthUrl, getLogsUrl, getNetworkUrl, getUpdatesyUrl, getServicesOverviewUrl, getServiceDetailUrl, getSetupUrl } from '@/assets/endpoints/app/appEndpoints';
import FrameActions from '@/main/frame/frameActions';
import Updates from '@/main/updates/updates';
import Network from '@/main/network/network';
import General from '@/main/general/general';
import DisplayHealthPage from '@/main/displayHealth/displayHealth';
import FrameLogs from '@/main/monitoring/frameLogs';
import ServicesOverview from '@/main/services/servicesOverview';
import ServiceDetail from '@/main/services/serviceDetail';
import { ServiceType } from '@/types';
import NotFound from '@/common/components/notFound';
import { createBrowserRouter } from 'react-router';
import { getAuthenticationUrl, getSignInUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import AuthenticationLayout from '@/main/authentication/layout';
import SignIn from '@/main/authentication/signIn/signIn';
import SignOut from '@/main/authentication/signOut/signOut';
import RouterContext from '@/common/components/routerContext';
import WifiSetup from '@/main/setup/wifiSetup';

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
            // Unauthenticated offline WiFi-setup page (board hosting its AP).
            path: getSetupUrl(),
            element: <WifiSetup />,
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
                    path: getDisplayHealthUrl(),
                    element: <DisplayHealthPage />
                  },
                  {
                    path: getLogsUrl(),
                    element: <FrameLogs />
                  },
                  {
                    path: getServicesOverviewUrl(),
                    element: <ServicesOverview />
                  },
                  {
                    path: getServiceDetailUrl('display'),
                    element: <ServiceDetail serviceId={ServiceType.DISPLAY} />
                  },
                  {
                    path: getServiceDetailUrl('websocket'),
                    element: <ServiceDetail serviceId={ServiceType.WEBSOCKET} />
                  },
                  {
                    path: getServiceDetailUrl('dashboard'),
                    element: <ServiceDetail serviceId={ServiceType.DASHBOARD} />
                  },
                  {
                    path: getServiceDetailUrl('heartbeat'),
                    element: <ServiceDetail serviceId={ServiceType.HEARTBEAT} />
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
