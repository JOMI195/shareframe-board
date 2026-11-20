// Latin only, the board ships every font file it imports; other scripts use the system font.
import '@fontsource/inter/latin-400.css'
import '@fontsource/inter/latin-ext-400.css'
import '@fontsource/inter/latin-500.css'
import '@fontsource/inter/latin-ext-500.css'
import '@fontsource/inter/latin-600.css'
import '@fontsource/inter/latin-ext-600.css'
import '@fontsource/inter/latin-700.css'
import '@fontsource/inter/latin-ext-700.css'
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
