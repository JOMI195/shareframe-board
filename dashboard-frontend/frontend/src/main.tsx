import '@fontsource/inter'
import { createRoot } from 'react-dom/client'
import { ColorThemeProvider } from './context/colorTheme/colorThemeContext.tsx'
import { CssBaseline } from '@mui/material'
import { Provider } from 'react-redux'
import { store } from './store/index.ts'
import { PiConnectionProvider } from './context/piConnection/piConnectionContext.tsx'
import App from './App.tsx'

createRoot(document.getElementById('root')!).render(
  <Provider store={store}>
    <ColorThemeProvider>
      <CssBaseline enableColorScheme />
      <PiConnectionProvider>
        <App />
      </PiConnectionProvider>
    </ColorThemeProvider>
  </Provider>
)
