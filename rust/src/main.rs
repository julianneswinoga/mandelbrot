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

use ggez::graphics::DrawMode;
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

fn update_mandel(
    ctx: &mut Context,
    top_left_scale: Complex<FloatPrecision>,
    bottom_right_scale: Complex<FloatPrecision>,
) -> graphics::Image {
    let mut pix_img = PixelImage::new(WIN_WIDTH, WIN_HEIGHT);

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
    let image =
        graphics::Image::from_rgba8(ctx, pix_img.width as u16, pix_img.height as u16, &flattened)
            .unwrap();

    image
}

#[derive(Default)]
struct ViewWindow {
    selecting: bool,
    top: usize,
    left: usize,
    bottom: usize,
    right: usize,
}

struct MainState {
    view_window: ViewWindow,
    top_left_graph: Complex<FloatPrecision>,
    last_top_left_graph: Complex<FloatPrecision>,
    bottom_right_graph: Complex<FloatPrecision>,
    last_bottom_right_graph: Complex<FloatPrecision>,
    image: graphics::Image,
}

impl MainState {
    fn new(ctx: &mut Context) -> GameResult<MainState> {
        let default_image =
            graphics::Image::solid(ctx, WIN_WIDTH as u16, ggez::graphics::BLACK).unwrap();

        let default_top_left_scale = Complex::<FloatPrecision>::new(-2.5, -1.0);
        let default_bottom_right_scale = Complex::<FloatPrecision>::new(1.0, 1.0);

        let s = MainState {
            view_window: ViewWindow::default(),
            top_left_graph: default_top_left_scale,
            last_top_left_graph: Complex::<FloatPrecision>::new(0.0, 0.0),
            bottom_right_graph: default_bottom_right_scale,
            last_bottom_right_graph: Complex::<FloatPrecision>::new(0.0, 0.0),
            image: default_image,
        };

        Ok(s)
    }
}

impl event::EventHandler for MainState {
    fn update(&mut self, ctx: &mut Context) -> GameResult {
        const DESIRED_FPS: u32 = 30;
        while timer::check_update_time(ctx, DESIRED_FPS) {
            if self.top_left_graph != self.last_top_left_graph
                || self.bottom_right_graph != self.last_bottom_right_graph
            {
                println!("Updating");
                self.image = update_mandel(ctx, self.top_left_graph, self.bottom_right_graph);
                self.last_top_left_graph = self.top_left_graph;
                self.last_bottom_right_graph = self.bottom_right_graph;
                println!("Done");
            }
        }
        Ok(())
    }

    fn draw(&mut self, ctx: &mut Context) -> GameResult {
        let clear_color = [0.0, 0.0, 0.0, 1.0].into();
        let default_params = ([0.0, 0.0], 0.0, [1.0, 1.0, 1.0, 1.0].into());

        graphics::clear(ctx, clear_color);

        graphics::draw(ctx, &self.image, default_params)?;

        if self.view_window.selecting {
            let x = self.view_window.left as f32;
            let y = self.view_window.top as f32;
            let w = (self.view_window.right as isize - self.view_window.left as isize) as f32;
            let h = (self.view_window.bottom as isize - self.view_window.top as isize) as f32;
            let outer_rect = graphics::Rect::new(x, y, w, h);
            let inner_rect = graphics::Rect::new(x + 1.0, y + 1.0, w - 2.0, h - 2.0);
            let rect_stroke = DrawMode::stroke(1.0);

            let outer_rect_mesh = graphics::Mesh::new_rectangle(
                ctx,
                rect_stroke,
                outer_rect,
                [0.0, 0.0, 0.0, 1.0].into(),
            )?;
            let inner_rect_mesh = graphics::Mesh::new_rectangle(
                ctx,
                rect_stroke,
                inner_rect,
                [1.0, 1.0, 1.0, 1.0].into(),
            )?;

            graphics::draw(ctx, &outer_rect_mesh, default_params)?;
            graphics::draw(ctx, &inner_rect_mesh, default_params)?;
        }

        graphics::present(ctx)?;

        timer::yield_now();
        Ok(())
    }

    fn mouse_button_down_event(
        &mut self,
        _ctx: &mut Context,
        _btn: event::MouseButton,
        x: f32,
        y: f32,
    ) {
        println!("Click at {} {} {:?}", x, y, _btn);
        match _btn {
            event::MouseButton::Left => {
                self.view_window.selecting = true;
                self.view_window.left = x as usize;
                self.view_window.top = y as usize;
                self.view_window.right = x as usize;
                self.view_window.bottom = y as usize;
            }
            _ => println!("Unhandled button press {:?}", _btn),
        };
    }

    fn mouse_button_up_event(
        &mut self,
        _ctx: &mut Context,
        _btn: event::MouseButton,
        x: f32,
        y: f32,
    ) {
        println!("Release at {} {} {:?}", x, y, _btn);
        match _btn {
            event::MouseButton::Left => {
                self.view_window.selecting = false;
                self.view_window.right = x as usize;
                self.view_window.bottom = y as usize;
                self.view_window.left = x as usize;
                self.view_window.top = y as usize;
            }
            _ => println!("Unhandled button press {:?}", _btn),
        };
    }

    fn mouse_motion_event(&mut self, _ctx: &mut Context, x: f32, y: f32, _dx: f32, _dy: f32) {
        if self.view_window.selecting {
            self.view_window.right = x as usize;
            self.view_window.bottom = y as usize;
        }
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
