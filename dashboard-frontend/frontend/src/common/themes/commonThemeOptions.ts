import type { Theme } from "@mui/material/styles";
import type { GridProps } from "@mui/material/Grid";

declare module "@mui/material/styles" {
  interface Theme {
    layout: {
      appbar: {
        height: number
      },
      footer: {
        height: number
      }
    };
  }

  interface ThemeOptions {
    layout: {
      appbar: {
        height: number
      },
      footer: {
        height: number
      }
    };
  }

  interface Palette {
    accent: {
      main: string;
    };
  }
  interface PaletteOptions {
    accent: {
      main: string;
    };
  }
}

export const commonThemeOptions = {
  components: {
    MuiCssBaseline: {
      styleOverrides: {
        '@media (prefers-reduced-motion: reduce)': {
          '*, *::before, *::after': {
            transitionDuration: '0.01ms !important',
            scrollBehavior: 'auto !important',
          },
        },
      },
    },
    MuiButton: {
      styleOverrides: {
        root: { textTransform: 'none' },
      },
    },
    MuiFab: {
      styleOverrides: {
        root: { textTransform: 'none' },
      },
    },
    MuiTab: {
      styleOverrides: {
        root: { textTransform: 'none' },
      },
    },
    MuiGrid: {
      styleOverrides: {
        container: ({ ownerState, theme }: { ownerState: GridProps; theme: Theme }) => ({
          width: '100%',
          ...(typeof ownerState.spacing === 'number' && ownerState.spacing > 0
            ? { paddingTop: theme.spacing(ownerState.spacing) }
            : {}),
        }),
      },
    },
  },
  typography: {
    fontFamily: [
      "Inter",
      "-apple-system",
      "BlinkMacSystemFont",
      '"Segoe UI"',
      "Roboto",
      '"Helvetica Neue"',
      "Arial",
      "sans-serif"
    ].join(','),
  },
  shape: {
    borderRadius: 10,
  },
  layout: {
    appbar: {
      height: 60,
    },
    footer: {
      height: 50
    }
  },
}