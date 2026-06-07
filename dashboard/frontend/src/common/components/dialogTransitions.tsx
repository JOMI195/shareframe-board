import { Slide, Zoom } from "@mui/material"
import { TransitionProps } from "@mui/material/transitions"
import React from "react"

export const ZoomTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<any, any>
    },
    ref: React.Ref<unknown>
) {
    return <Zoom ref={ref} {...props} />
})

export const SlideTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<any, any>
    },
    ref: React.Ref<unknown>
) {
    return <Slide ref={ref} {...props} />
})