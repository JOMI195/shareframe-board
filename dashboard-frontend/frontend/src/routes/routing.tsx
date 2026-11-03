import MainLayout from '@/common/components/layout/layout';
import Snackbars from '@/common/components/snackbars/snackbars';
import ProtectedRoute from '@/common/components/protectedRoute';
import { getHomeUrl, getSlideshowPageUrl, getCustomizationUrl, getGeneralSettingsUrl, getDisplayHealthUrl, getLogsUrl, getNetworkUrl, getUpdatesyUrl, getUpdateHistoryPageUrl, getServicesOverviewUrl, getServiceDetailUrl } from '@/assets/endpoints/app/appEndpoints';
import FrameActions from '@/main/frame/frameActions';
import { ServiceType } from '@/types';
import NotFound from '@/common/components/error/notFound/notFound';
import RouteError from '@/common/components/error/routeError/routeError';
import LoadingFallback from '@/common/components/loadingFallback';
import { createBrowserRouter } from 'react-router';
import { getAuthenticationUrl, getSignInUrl, getSignOutUrl } from '@/assets/endpoints/app/authEndpoints';
import AuthenticationLayout from '@/main/authentication/layout';
import SignIn from '@/main/authentication/signIn/signIn';
import SignOut from '@/main/authentication/signOut/signOut';
import RouterContext from '@/common/components/routerContext';
import { lazyPage } from './lazyPage';

const serviceDetail = (serviceId: ServiceType) => ({
  lazy: async () => {
    const { default: ServiceDetail } = await import('@/main/services/serviceDetail');
    return { element: <ServiceDetail serviceId={serviceId} /> };
  },
});

// eslint-disable-next-line react-refresh/only-export-components
export const routes = [
  {
    element: <RouterContext />,
    errorElement: <RouteError />,
    HydrateFallback: LoadingFallback,
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
                    errorElement: <RouteError />,
                    children: [
                      {
                        path: "*",
                        element: <NotFound />,
                      },
                      {
                        path: getHomeUrl(),
                        handle: { title: 'Steuerung' },
                        element: <FrameActions />,
                      },
                      {
                        path: getSlideshowPageUrl(),
                        handle: { title: 'Diashow' },
                        ...lazyPage(() => import('@/main/slideshowImages/slideshowImages')),
                      },
                      {
                        path: getCustomizationUrl(),
                        handle: { title: 'Anpassung' },
                        ...lazyPage(() => import('@/main/customization/customization')),
                      },
                      {
                        path: getNetworkUrl(),
                        handle: { title: 'Netzwerk' },
                        ...lazyPage(() => import('@/main/network/network')),
                      },
                      {
                        path: getUpdatesyUrl(),
                        handle: { title: 'Updates' },
                        ...lazyPage(() => import('@/main/updates/updates')),
                      },
                      {
                        path: getUpdateHistoryPageUrl(),
                        handle: { title: 'Update-Verlauf' },
                        ...lazyPage(() => import('@/main/updateHistory/updateHistory')),
                      },
                      {
                        path: getGeneralSettingsUrl(),
                        handle: { title: 'System' },
                        ...lazyPage(() => import('@/main/general/general')),
                      },
                      {
                        path: getDisplayHealthUrl(),
                        handle: { title: 'Display-Zustand' },
                        ...lazyPage(() => import('@/main/displayHealth/displayHealth')),
                      },
                      {
                        path: getLogsUrl(),
                        handle: { title: 'Protokolle' },
                        ...lazyPage(() => import('@/main/monitoring/frameLogs')),
                      },
                      {
                        path: getServicesOverviewUrl(),
                        handle: { title: 'Dienste' },
                        ...lazyPage(() => import('@/main/services/servicesOverview')),
                      },
                      { path: getServiceDetailUrl('display'), handle: { title: 'Display' }, ...serviceDetail(ServiceType.DISPLAY) },
                      { path: getServiceDetailUrl('websocket'), handle: { title: 'WebSocket' }, ...serviceDetail(ServiceType.WEBSOCKET) },
                      { path: getServiceDetailUrl('dashboard'), handle: { title: 'Dashboard' }, ...serviceDetail(ServiceType.DASHBOARD) },
                      { path: getServiceDetailUrl('heartbeat'), handle: { title: 'Heartbeat' }, ...serviceDetail(ServiceType.HEARTBEAT) },
                      { path: getServiceDetailUrl('update'), handle: { title: 'Update-Dienst' }, ...serviceDetail(ServiceType.UPDATE) },
                    ],
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
