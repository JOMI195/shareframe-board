import { createContext, ReactNode, useContext, useState } from 'react';
import { createPortal } from 'react-dom';
import { Box, Snackbar, SnackbarProps } from '@mui/material';

const StackContext = createContext<HTMLElement | null>(null);

// One fixed column for all app snackbars, so several at once stack instead of covering each other.
export const SnackbarStackProvider = ({ children }: { children: ReactNode }) => {
  const [stack, setStack] = useState<HTMLElement | null>(null);

  return (
    <StackContext.Provider value={stack}>
      {children}
      <Box
        ref={setStack}
        sx={(theme) => ({
          position: 'fixed',
          // Above the page's floating action buttons on phones.
          bottom: {
            xs: 'calc(88px + env(safe-area-inset-bottom))',
            sm: 'calc(16px + env(safe-area-inset-bottom))',
          },
          left: 0,
          right: 0,
          px: 1,
          zIndex: theme.zIndex.snackbar,
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          gap: 1,
          pointerEvents: 'none',
          '& > *': { pointerEvents: 'auto' },
        })}
      />
    </StackContext.Provider>
  );
};

export const StackedSnackbar = (props: Omit<SnackbarProps, 'anchorOrigin'>) => {
  const stack = useContext(StackContext);
  if (!stack) return <Snackbar anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }} {...props} />;

  return createPortal(
    <Snackbar
      {...props}
      sx={{
        position: 'static',
        transform: 'none',
        justifyContent: 'center',
        width: { xs: '100%', sm: 'auto' },
        maxWidth: '100%',
      }}
    />,
    stack,
  );
};
