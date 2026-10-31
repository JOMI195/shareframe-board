import '@fontsource/inter'
import '@fontsource/inter/500.css'
import '@fontsource/inter/600.css'
import '@fontsource/inter/700.css'
import { createRoot } from 'react-dom/client'
import { ColorThemeProvider } from './context/colorTheme/colorThemeContext.tsx'
import { CssBaseline } from '@mui/material'
import { Provider } from 'react-redux'
import { store } from './store/index.ts'
import { PiConnectionProvider } from './context/piConnection/piConnectionContext.tsx'
import App from './App.tsx'
import ErrorBoundary from './common/components/error/errorBoundary/errorBoundary.tsx'
import { initStaleChunkReload } from './common/utils/staleChunkReload.ts'

initStaleChunkReload()

createRoot(document.getElementById('root')!).render(
  <ErrorBoundary>
    <Provider store={store}>
      <ColorThemeProvider>
        <CssBaseline enableColorScheme />
        <PiConnectionProvider>
          <App />
        </PiConnectionProvider>
      </ColorThemeProvider>
    </Provider>
  </ErrorBoundary>
)
