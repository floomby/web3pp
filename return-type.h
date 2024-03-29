// Adapted from code I shamefully stole from stack overflow

#pragma once

template <typename F> struct return_type_impl { using type = void; };

template <typename R, typename... Args> struct return_type_impl<R(Args...)> {
    using type = R;
};

template <typename R, typename... Args>
struct return_type_impl<R(Args..., ...)> {
    using type = R;
};

template <typename R, typename... Args>
struct return_type_impl<R (*)(Args...)> {
    using type = R;
};

template <typename R, typename... Args>
struct return_type_impl<R (*)(Args..., ...)> {
    using type = R;
};

template <typename R, typename... Args>
struct return_type_impl<R (&)(Args...)> {
    using type = R;
};

template <typename R, typename... Args>
struct return_type_impl<R (&)(Args..., ...)> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...)> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...)> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) volatile> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) volatile> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) volatile &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) volatile &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) volatile &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) volatile &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const volatile> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const volatile> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const volatile &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const volatile &> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args...) const volatile &&> {
    using type = R;
};

template <typename R, typename C, typename... Args>
struct return_type_impl<R (C::*)(Args..., ...) const volatile &&> {
    using type = R;
};

template <typename T, typename = void>
struct return_type : return_type_impl<T> {};

template <typename T>
struct return_type<T, decltype(void(&T::operator()))> : return_type_impl<decltype(&T::operator())> {};

template <typename T> using return_type_t = typename return_type<T>::type;
