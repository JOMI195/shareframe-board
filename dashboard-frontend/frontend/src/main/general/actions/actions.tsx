import { useAppDispatch } from "@/store";
import { DialogAction } from "@/types";
import BottomFloatingActions from "@/common/components/bottomFloatingActions";
import { openRestartDialog, openShutdownDialog } from "@/store/dialogs/dialogs.Slice";
import RestartAltIcon from '@mui/icons-material/RestartAlt';
import PowerSettingsNewIcon from '@mui/icons-material/PowerSettingsNew';

export const Actions: React.FC = () => {
  const dispatch = useAppDispatch();

  const handleRestartDialogOpen = () => {
    dispatch(openRestartDialog());
  };

  const handleShutdownDialogOpen = () => {
    dispatch(openShutdownDialog());
  };

  // Surface shutdown as a labeled extended Fab so the power button reads
  // "Herunterfahren" (was an icon-only SpeedDial). Restart sits beside it.
  const shutdownAction: DialogAction = {
    icon: <PowerSettingsNewIcon />,
    onClick: handleShutdownDialogOpen,
    label: 'Herunterfahren',
    color: 'error',
  };

  const restartAction: DialogAction = {
    icon: <RestartAltIcon />,
    onClick: handleRestartDialogOpen,
    label: 'Neustarten',
    color: 'primary',
  };

  return (
    <BottomFloatingActions
      actionPrimary={shutdownAction}
      actionSecondary={restartAction}
    />
  );
};
