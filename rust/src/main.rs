extern crate ggez;
#[macro_use]
extern crate lazy_static;
extern crate num_complex;

use ggez::conf::{WindowMode, WindowSetup};
use ggez::event;
use ggez::graphics;
use ggez::timer;
use ggez::{Context, GameResult};
use num_complex::Complex;
use num_traits::pow::Pow;

use pixeling::*;

mod pixeling;

type FloatPrecision = f32;

static WIN_WIDTH: usize = 640;
static WIN_HEIGHT: usize = 480;
static MAX_ITER: usize = 1000;

fn linear_transform<T>(x: T, a: T, b: T, c: T, d: T) -> T
where
    T: Copy
        + std::ops::Sub<T, Output = T>
        + std::ops::Div<T, Output = T>
        + std::ops::Mul<T, Output = T>
        + std::ops::Add<T, Output = T>,
{
    (x - a) / (b - a) * (d - c) + c
}

fn pix_to_cmplx(
    top_left_scale: Complex<FloatPrecision>,
    bottom_right_scale: Complex<FloatPrecision>,
    x: usize,
    y: usize,
) -> Complex<FloatPrecision> {
    let re = linear_transform::<FloatPrecision>(
        x as FloatPrecision,
        0 as FloatPrecision,
        WIN_WIDTH as FloatPrecision,
        top_left_scale.re,
        bottom_right_scale.re,
    );

    let im = linear_transform::<FloatPrecision>(
        y as FloatPrecision,
        0 as FloatPrecision,
        WIN_HEIGHT as FloatPrecision,
        top_left_scale.im,
        bottom_right_scale.im,
    );

    Complex::new(re, im)
}

struct MainState {
    top_left_scale: Complex<FloatPrecision>,
    bottom_right_scale: Complex<FloatPrecision>,
    image: graphics::Image,
}

impl MainState {
    fn new(ctx: &mut Context) -> GameResult<MainState> {
        let mut pix_img = PixelImage::new(WIN_WIDTH, WIN_HEIGHT);
        let top_left_scale: Complex<FloatPrecision> = Complex::new(-2.5, -1.0);
        let bottom_right_scale: Complex<FloatPrecision> = Complex::new(1.0, 1.0);

        for i in 0..pix_img.width {
            for j in 0..pix_img.height {
                let point0 = pix_to_cmplx(top_left_scale, bottom_right_scale, i, j);
                let mut point = Complex::<FloatPrecision>::new(0.0, 0.0);

                let mut iteration: usize = 0;
                while (point * point).re <= 2.0 * 2.0 && iteration < MAX_ITER {
                    let re_temp = point.re * point.re - point.im * point.im + point0.re;
                    point.im = 2.0 * point.re * point.im + point0.im;
                    point.re = re_temp;

                    iteration += 1;
                }

                let pix = RgbaPixel::from_rainbow(iteration);

                pix_img.pixels[j][i].set(pix.r, pix.g, pix.b, pix.a);
            }
        }

        let flattened: Vec<u8> = pix_img.flat();
        let image = graphics::Image::from_rgba8(
            ctx,
            pix_img.width as u16,
            pix_img.height as u16,
            &flattened,
        )
        .unwrap();

        let s = MainState {
            top_left_scale,
            bottom_right_scale,
            image,
        };

        Ok(s)
    }
}

impl event::EventHandler for MainState {
    fn update(&mut self, ctx: &mut Context) -> GameResult {
        const DESIRED_FPS: u32 = 30;
        while timer::check_update_time(ctx, DESIRED_FPS) {}
        Ok(())
    }

    fn draw(&mut self, ctx: &mut Context) -> GameResult {
        graphics::clear(ctx, [0.0, 0.0, 0.0, 1.0].into());

        graphics::draw(
            ctx,
            &self.image,
            ([0.0, 0.0], 0.0, [1.0, 1.0, 1.0, 1.0].into()),
        )?;

        graphics::present(ctx)?;

        timer::yield_now();
        Ok(())
    }
}

pub fn main() -> GameResult {
    let cb = ggez::ContextBuilder::new("", "")
        .window_setup(WindowSetup::default().title("rusty mandelbrot"))
        .window_mode(
            WindowMode::default()
                .dimensions(WIN_WIDTH as f32, WIN_HEIGHT as f32)
                .resizable(false),
        );

    let (ctx, event_loop) = &mut cb.build()?;

    let state = &mut MainState::new(ctx)?;
    event::run(ctx, event_loop, state)
}
