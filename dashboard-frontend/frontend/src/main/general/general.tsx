import {
    Stack,
} from '@mui/material';
import { useAppSelector } from '@/store';
import { selectFrameInfoState } from '@/store/frameInfo/frameInfo.Slice';
import Dialogs from './dialogs/dialogs';
import { Actions } from './actions/actions';
import ShareframeInfoCard from '@/common/components/shareframeInfoCard';

const RESOLUTION = "800mm x 480mm";

const General = () => {
    const frameInfos = useAppSelector(selectFrameInfoState).frameInfo;

    return (
        <>
            <Stack width={"100%"} spacing={3}>
                <ShareframeInfoCard
                    title="Gerät"
                    sections={[
                        {
                            label: "Seriennummer",
                            content: frameInfos?.public_serial_number

                        },
                    ]}
                />

                <ShareframeInfoCard
                    title="Hardwarespezifikation"
                    sections={[
                        {
                            label: "Display-Größe",
                            content: RESOLUTION
                        },
                    ]}
                />
            </Stack >
            <Actions />
            <Dialogs />
        </>
    );
};

export default General;