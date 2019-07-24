use num_complex::Complex;
use std::ops::{Add, Div, Mul, Sub};

pub type FloatPrecision = f32;

pub fn linear_transform<T>(x: T, a: T, b: T, c: T, d: T) -> T
where
    T: Copy + Add<T, Output = T> + Div<T, Output = T> + Mul<T, Output = T> + Sub<T, Output = T>,
{
    (x - a) / (b - a) * (d - c) + c
}

pub fn pix_to_cmplx(
    top_left_scale: Complex<FloatPrecision>,
    bottom_right_scale: Complex<FloatPrecision>,
    x: usize,
    y: usize,
    win_width: usize,
    win_height: usize,
) -> Complex<FloatPrecision> {
    let re = linear_transform::<FloatPrecision>(
        x as FloatPrecision,
        0 as FloatPrecision,
        win_width as FloatPrecision,
        top_left_scale.re,
        bottom_right_scale.re,
    );

    let im = linear_transform::<FloatPrecision>(
        y as FloatPrecision,
        0 as FloatPrecision,
        win_height as FloatPrecision,
        top_left_scale.im,
        bottom_right_scale.im,
    );

    Complex::new(re, im)
}

pub fn compute_iter_for_point(position: Complex<FloatPrecision>, max_iter: usize) -> usize {
    // Cardioid test
    let q = (position.re - 0.25).powi(2) + position.im.powi(2);
    if q * (q + (position.re - 0.25)) <= 0.25 * position.im.powi(2) {
        return max_iter;
    }

    // Period-2 bulb test
    if (position.re + 1.0).powi(2) + position.im.powi(2) < 1.0 / 16.0 {
        return max_iter;
    }

    let mut point = Complex::<FloatPrecision>::new(0.0, 0.0);
    let mut iteration: usize = 0;
    while point.re.powi(2) <= 2.0 * 2.0 && iteration < max_iter {
        let re_temp = point.re.powi(2) - point.im.powi(2) + position.re;
        point.im = 2.0 * point.re * point.im + position.im;
        point.re = re_temp;

        iteration += 1;
    }

    iteration
}
