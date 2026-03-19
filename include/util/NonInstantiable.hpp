#pragma once

struct NonInstantiable
{
    NonInstantiable() = delete;
    NonInstantiable(const NonInstantiable&) = delete;
    NonInstantiable(NonInstantiable&&) = delete;
    NonInstantiable& operator=(const NonInstantiable&) = delete;
    NonInstantiable& operator=(NonInstantiable&&) = delete;
};
