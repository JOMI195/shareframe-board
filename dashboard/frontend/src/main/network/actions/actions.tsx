import React from 'react';
import { useAppDispatch, useAppSelector } from "@/store";
import { DialogAction } from "@/types";
import BottomFloatingActions from "@/common/components/bottomFloatingActions";
import { openNetworkAddNetworkDialog } from "@/store/dialogs/dialogs.Slice";
import { fetchNetworkData, selectNetworkState } from '@/store/network/network.Slice';
import { usePiConnection } from '@/context/piConnection/piConnectionContext';
import AddIcon from '@mui/icons-material/Add';
import RefreshIcon from '@mui/icons-material/Refresh';
import uuid from 'react-uuid';
import { addLoadingSnackbar, removeLoadingSnackbar } from '@/store/snackbars/snackbars.Slice';

export const Actions: React.FC = () => {
  const dispatch = useAppDispatch();
  const { isConnected } = usePiConnection();
  const { loading } = useAppSelector(selectNetworkState);

  const isButtonsDisabled = loading || !isConnected;

  const handleAddNetworkDialogOpen = () => {
    dispatch(openNetworkAddNetworkDialog());
  };

  const handleRefreshNetworks = async () => {
    if (isConnected) {
      const snackbarId = uuid();
      dispatch(addLoadingSnackbar(snackbarId, "Lade Netzwerke"));
      await dispatch(fetchNetworkData());
      dispatch(removeLoadingSnackbar(snackbarId));
    }
  };

  const primaryAction: DialogAction = {
    icon: <AddIcon />,
    onClick: handleAddNetworkDialogOpen,
    label: 'Netzwerk hinzufügen',
    color: 'primary',
    disabled: isButtonsDisabled,
  };

  const secondaryAction: DialogAction = {
    icon: <RefreshIcon />,
    onClick: handleRefreshNetworks,
    color: 'primary',
    disabled: isButtonsDisabled,
  };

  return (
    <BottomFloatingActions
      actionPrimary={primaryAction}
      actionSecondary={secondaryAction}
    />
  );
};