extern crate ggez;
extern crate rand;

use ggez::conf::{WindowMode, WindowSetup};
use ggez::event;
use ggez::graphics;
use ggez::timer;
use ggez::{Context, GameResult};

static WIN_WIDTH: usize = 640;
static WIN_HEIGHT: usize = 480;

#[derive(Debug, Clone, Copy)]
struct RgbaPixel {
    r: u8,
    g: u8,
    b: u8,
    a: u8,
}

impl Default for RgbaPixel {
    fn default() -> Self {
        RgbaPixel {
            r: 0,
            g: 0,
            b: 0,
            a: 0xff,
        }
    }
}

impl RgbaPixel {
    fn new(values: Option<[u8; 4]>) -> RgbaPixel {
        match values {
            Some(values) => RgbaPixel {
                r: values[0],
                g: values[1],
                b: values[2],
                a: values[3],
            },
            None => RgbaPixel {
                ..Default::default()
            },
        }
    }

    fn set(&mut self, r: u8, g: u8, b: u8, a: u8) {
        self.r = r;
        self.g = g;
        self.b = b;
        self.a = a;
    }

    fn flat(&self) -> [u8; 4] {
        [self.r, self.g, self.b, self.a]
    }
}

struct PixelImage {
    width: usize,
    height: usize,
    pixels: Vec<Vec<RgbaPixel>>,
}

impl PixelImage {
    fn new(width: usize, height: usize) -> PixelImage {
        PixelImage {
            width,
            height,
            pixels: vec![vec![RgbaPixel::new(None); width]; height],
        }
    }

    fn flat(&mut self) -> Vec<u8> {
        let mut rtn: Vec<u8> = vec![];
        for row in self.pixels.iter() {
            for pix in row.iter() {
                let flattened = pix.flat();
                rtn.extend_from_slice(&flattened);
            }
        }
        rtn
    }
}

struct MainState {
    image: graphics::Image,
}

impl MainState {
    fn new(ctx: &mut Context) -> GameResult<MainState> {
        let mut pix_img = PixelImage::new(WIN_WIDTH, WIN_HEIGHT);

        for i in 0..pix_img.width {
            for j in 0..pix_img.height {
                let r: u8 = (((i + 1) as f32 / pix_img.width as f32) * 0xff as f32) as u8;
                let g: u8 = (((j + 1) as f32 / pix_img.height as f32) * 0xff as f32) as u8;
                pix_img.pixels[j][i].set(r, g, 0, 0xff);
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
