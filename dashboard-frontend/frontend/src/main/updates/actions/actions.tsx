import { useAppDispatch, useAppSelector } from "@/store";
import { DialogAction } from "@/types";
import BottomFloatingActions from "@/common/components/bottomFloatingActions";
import { openUpdatesConfirmUpdateDialog } from "@/store/dialogs/dialogs.Slice";
import BuildIcon from '@mui/icons-material/Build';
import RefreshIcon from '@mui/icons-material/Refresh';
import { fetchLatestRelease, selectUpdatesState } from "@/store/updates/updates.Slice";
import { isVersionNewer } from "@/common/utils/version";
import { selectFrameInfoState } from "@/store/frameInfo/frameInfo.Slice";
import uuid from "react-uuid";
import { addLoadingSnackbar, removeLoadingSnackbar } from "@/store/snackbars/snackbars.Slice";
import { usePiConnection } from "@/context/piConnection/piConnectionContext";

export const Actions: React.FC = () => {
  const dispatch = useAppDispatch();

  const { latest_release, update_status, loading } = useAppSelector(selectUpdatesState);
  const { frameInfo } = useAppSelector(selectFrameInfoState);
  const { isConnected } = usePiConnection();

  const isNewVersion = (latest_release && frameInfo && isVersionNewer(latest_release.version, frameInfo.version)) ?? false;
  const updateBusy = update_status != null && (
    ['checking', 'downloading', 'installing', 'awaiting-reboot'].includes(update_status.phase)
    || (update_status.pending_slot !== '' && update_status.pending_slot === update_status.booted_slot)
  );

  const handleConfirmUpdateButtonClicked = () => {
    dispatch(openUpdatesConfirmUpdateDialog());
  };

  const handleFetchLatestReleaseButtonClicked = async () => {
    const snackbarId = uuid();
    dispatch(addLoadingSnackbar(snackbarId, "Suche nach Updates"));
    await dispatch(fetchLatestRelease());
    dispatch(removeLoadingSnackbar(snackbarId));
  }

  const confirmUpdateAction: DialogAction = {
    icon: <BuildIcon />,
    onClick: handleConfirmUpdateButtonClicked,
    label: 'Jetzt installieren',
    color: 'primary',
    disabled: loading || !isConnected || updateBusy
  };

  const fetchLatestReleaseAction: DialogAction = {
    icon: <RefreshIcon />,
    onClick: handleFetchLatestReleaseButtonClicked,
    label: 'Auf Updates prüfen',
    color: 'primary',
    disabled: loading || !isConnected
  };

  if (isNewVersion) {
    return (
      <BottomFloatingActions
        actionPrimary={confirmUpdateAction}
      />
    )
  } else {
    return (
      <BottomFloatingActions
        actionPrimary={fetchLatestReleaseAction}
      />
    )
  }
};
