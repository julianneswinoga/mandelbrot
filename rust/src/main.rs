extern crate ggez;
extern crate rand;
#[macro_use]
extern crate lazy_static;

use ggez::conf::{WindowMode, WindowSetup};
use ggez::event;
use ggez::graphics;
use ggez::timer;
use ggez::{Context, GameResult};

mod pixeling;
use pixeling::*;

static WIN_WIDTH: usize = 640;
static WIN_HEIGHT: usize = 480;

struct MainState {
    image: graphics::Image,
}

impl MainState {
    fn new(ctx: &mut Context) -> GameResult<MainState> {
        let mut pix_img = PixelImage::new(WIN_WIDTH, WIN_HEIGHT);

        for i in 0..pix_img.width {
            for j in 0..pix_img.height {
                let pix = RgbaPixel::from_rainbow(i);

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

        let s = MainState { image };

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
