import { ReactNode } from 'react';
import { Provider } from 'react-redux';
import { ColorThemeProvider } from '@/context/colorTheme/colorThemeContext';
import { PiConnectionProvider } from '@/context/piConnection/piConnectionContext';
import type { TestStore } from '@/store/setupStore';

export const AppProviders = ({ store, children }: { store: TestStore; children: ReactNode }) => (
  <Provider store={store}>
    <ColorThemeProvider>
      <PiConnectionProvider>{children}</PiConnectionProvider>
    </ColorThemeProvider>
  </Provider>
);
