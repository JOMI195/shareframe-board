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

  const secondaryActions: DialogAction[] = [
    {
      icon: <PowerSettingsNewIcon />,
      onClick: handleShutdownDialogOpen,
      label: 'Herunterfahren',
      color: 'error',
    },
    {
      icon: <RestartAltIcon />,
      onClick: handleRestartDialogOpen,
      label: 'Neustarten',
      color: 'primary',
    }
  ];

  return (
    <BottomFloatingActions
      actionsAdditional={secondaryActions}
      speedDialIcon={<PowerSettingsNewIcon />}
    />
  );
};
