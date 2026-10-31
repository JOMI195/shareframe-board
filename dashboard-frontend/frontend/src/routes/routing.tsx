import MainLayout from '@/common/components/layout/layout';
import Snackbars from '@/common/components/snackbars/snackbars';
import ProtectedRoute from '@/common/components/protectedRoute';
import { getHomeUrl, getSlideshowPageUrl, getCustomizationUrl, getGeneralSettingsUrl, getDisplayHealthUrl, getLogsUrl, getNetworkUrl, getUpdatesyUrl, getUpdateHistoryPageUrl, getServicesOverviewUrl, getServiceDetailUrl } from '@/assets/endpoints/app/appEndpoints';
import FrameActions from '@/main/frame/frameActions';
import SlideshowImages from '@/main/slideshowImages/slideshowImages';
import Customization from '@/main/customization/customization';
import Updates from '@/main/updates/updates';
import UpdateHistory from '@/main/updateHistory/updateHistory';
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

// eslint-disable-next-line react-refresh/only-export-components
export const routes = [
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
                handle: { title: 'Anmelden' },
                element: <SignIn />,
              },
              {
                path: getAuthenticationUrl() + getSignOutUrl(),
                handle: { title: 'Abmelden' },
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
                    handle: { title: 'Steuerung' },
                    element: <FrameActions />
                  },
                  {
                    path: getSlideshowPageUrl(),
                    handle: { title: 'Diashow' },
                    element: <SlideshowImages />
                  },
                  {
                    path: getCustomizationUrl(),
                    handle: { title: 'Anpassung' },
                    element: <Customization />
                  },
                  {
                    path: getNetworkUrl(),
                    handle: { title: 'Netzwerk' },
                    element: <Network />
                  },
                  {
                    path: getUpdatesyUrl(),
                    handle: { title: 'Updates' },
                    element: <Updates />
                  },
                  {
                    path: getUpdateHistoryPageUrl(),
                    handle: { title: 'Update-Verlauf' },
                    element: <UpdateHistory />
                  },
                  {
                    path: getGeneralSettingsUrl(),
                    handle: { title: 'System' },
                    element: <General />
                  },
                  {
                    path: getDisplayHealthUrl(),
                    handle: { title: 'Display-Zustand' },
                    element: <DisplayHealthPage />
                  },
                  {
                    path: getLogsUrl(),
                    handle: { title: 'Protokolle' },
                    element: <FrameLogs />
                  },
                  {
                    path: getServicesOverviewUrl(),
                    handle: { title: 'Dienste' },
                    element: <ServicesOverview />
                  },
                  {
                    path: getServiceDetailUrl('display'),
                    handle: { title: 'Display' },
                    element: <ServiceDetail serviceId={ServiceType.DISPLAY} />
                  },
                  {
                    path: getServiceDetailUrl('websocket'),
                    handle: { title: 'WebSocket' },
                    element: <ServiceDetail serviceId={ServiceType.WEBSOCKET} />
                  },
                  {
                    path: getServiceDetailUrl('dashboard'),
                    handle: { title: 'Dashboard' },
                    element: <ServiceDetail serviceId={ServiceType.DASHBOARD} />
                  },
                  {
                    path: getServiceDetailUrl('heartbeat'),
                    handle: { title: 'Heartbeat' },
                    element: <ServiceDetail serviceId={ServiceType.HEARTBEAT} />
                  },
                  {
                    path: getServiceDetailUrl('update'),
                    handle: { title: 'Update-Dienst' },
                    element: <ServiceDetail serviceId={ServiceType.UPDATE} />
                  },
                ],
              },
            ]
          },
        ],
      }
    ]
  },
];

const Routing = createBrowserRouter(routes);

export default Routing;
