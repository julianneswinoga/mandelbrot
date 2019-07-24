extern crate ggez;
#[macro_use]
extern crate lazy_static;
extern crate num_complex;
extern crate structopt;

use ggez::conf::{WindowMode, WindowSetup};
use ggez::event;
use ggez::graphics;
use ggez::graphics::DrawMode;
use ggez::timer;
use ggez::{Context, GameResult};
use num_complex::Complex;
use structopt::StructOpt;

use calculations::*;
use pixeling::*;

mod calculations;
mod pixeling;

pub static WIN_WIDTH: usize = 640;
pub static WIN_HEIGHT: usize = 480;
pub static MAX_ITER: usize = 1000;

fn update_mandel(
    ctx: &mut Context,
    top_left_scale: Complex<FloatPrecision>,
    bottom_right_scale: Complex<FloatPrecision>,
) -> graphics::Image {
    let mut pix_img = PixelImage::new(WIN_WIDTH, WIN_HEIGHT);

    let max_width = pix_img.width;
    let max_height = pix_img.height;

    let pix_iterator = (0..max_width).flat_map(|x| (std::iter::repeat(x).zip(0..max_height)));

    for (x, y) in pix_iterator {
        let current_point = pix_to_cmplx(
            top_left_scale,
            bottom_right_scale,
            x,
            y,
            WIN_WIDTH,
            WIN_HEIGHT,
        );

        let pix = RgbaPixel::from_rainbow(compute_iter_for_point(current_point, MAX_ITER));

        pix_img.pixels[y][x].set(pix.r, pix.g, pix.b, pix.a);
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
    fn new(
        ctx: &mut Context,
        default_top_left_scale: Complex<FloatPrecision>,
        default_bottom_right_scale: Complex<FloatPrecision>,
    ) -> GameResult<MainState> {
        let default_image =
            graphics::Image::solid(ctx, WIN_WIDTH as u16, ggez::graphics::BLACK).unwrap();

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
                println!(
                    "Updating to {} {}",
                    self.top_left_graph, self.bottom_right_graph
                );
                let start_time = timer::time_since_start(ctx);
                self.image = update_mandel(ctx, self.top_left_graph, self.bottom_right_graph);
                self.last_top_left_graph = self.top_left_graph;
                self.last_bottom_right_graph = self.bottom_right_graph;
                println!("Done");
                let end_time = timer::time_since_start(ctx);
                println!("Update took {}ms", (end_time - start_time).as_millis());

                let window_title = format!(
                    "rusty mandelbrot - viewing {}{:+}i to {}{:+}i",
                    self.top_left_graph.re,
                    self.top_left_graph.im,
                    self.bottom_right_graph.re,
                    self.bottom_right_graph.im
                );
                graphics::set_window_title(ctx, &window_title);
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
        match _btn {
            event::MouseButton::Left => {
                self.view_window.selecting = false;

                // Setting the graph extents will force an update
                self.top_left_graph = pix_to_cmplx(
                    self.top_left_graph,
                    self.bottom_right_graph,
                    self.view_window.left as usize,
                    self.view_window.top as usize,
                    WIN_WIDTH,
                    WIN_HEIGHT,
                );
                self.bottom_right_graph = pix_to_cmplx(
                    self.top_left_graph,
                    self.bottom_right_graph,
                    self.view_window.right as usize,
                    self.view_window.bottom as usize,
                    WIN_WIDTH,
                    WIN_HEIGHT,
                );

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

fn parse_starting_scale(src: &str) -> Result<[FloatPrecision; 4], String> {
    let substrs: Vec<&str> = src
        .trim_matches(|c| c == '(' || c == ')')
        .split(',')
        .collect();
    if substrs.len() != 4 {
        return Err(format!("must only have 4 elements"));
    }

    let mut floats = Vec::<FloatPrecision>::new();
    for substr in substrs {
        floats.push(match substr.trim().parse::<FloatPrecision>() {
            Ok(f) => f,
            Err(e) => return Err(format!("Could not parse {:?}", e)),
        })
    }

    let rtn_floats: [FloatPrecision; 4] = [floats[0], floats[1], floats[2], floats[3]];
    Ok(rtn_floats)
}

#[derive(StructOpt, Debug)]
#[structopt(
    name = "mandelbrot",
    about = "a rusty implementation of a mandelbrot viewer"
)]
struct Opt {
    // The number of occurrences of the `v/verbose` flag
    /// Verbose mode (-v, -vv, -vvv, etc.)
    #[structopt(short = "v", long = "verbose", parse(from_occurrences))]
    verbose: u8,

    #[structopt(
        long = "starting_scale",
        help = "Set the starting screen to starting_scale. Coords are left,top,right,bottom",
        default_value = "(-2.5,-1.0,1.0,1.0)",
        parse(try_from_str = "parse_starting_scale")
    )]
    starting_scale: [FloatPrecision; 4],
}

pub fn main() -> GameResult {
    let opt = Opt::from_args();

    let default_top_left_scale =
        Complex::<FloatPrecision>::new(opt.starting_scale[0], opt.starting_scale[1]);
    let default_bottom_right_scale =
        Complex::<FloatPrecision>::new(opt.starting_scale[2], opt.starting_scale[3]);

    let cb = ggez::ContextBuilder::new("", "")
        .window_setup(WindowSetup::default())
        .window_mode(
            WindowMode::default()
                .dimensions(WIN_WIDTH as f32, WIN_HEIGHT as f32)
                .resizable(false),
        );

    let (ctx, event_loop) = &mut cb.build()?;

    let state = &mut MainState::new(ctx, default_top_left_scale, default_bottom_right_scale)?;
    event::run(ctx, event_loop, state)
}
