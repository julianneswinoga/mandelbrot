extern crate palette;

use palette::{Gradient, Hsv, Pixel, Srgb};

const RAINBOW_SIZE: usize = 256;

lazy_static! {
    static ref RAINBOW_GRADIENT: Vec<[u8; 3]> = Gradient::new(vec![
        Hsv::new(0.0, 1.0, 1.0),
        Hsv::new(180.0, 1.0, 1.0),
        Hsv::new(360.0, 1.0, 1.0)
    ])
    .take(RAINBOW_SIZE)
    .map(|x| { Srgb::from_linear(x.into()).into_format().into_raw() })
    .collect::<Vec<_>>();
}

#[derive(Debug, Clone, Copy)]
pub struct RgbaPixel {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
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
    pub fn new(values: Option<[u8; 4]>) -> RgbaPixel {
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

    pub fn from_rainbow(index: usize) -> RgbaPixel {
        let mod_index = index % RAINBOW_GRADIENT.len();
        let rgb_gradient = RAINBOW_GRADIENT[mod_index];

        RgbaPixel::new(Some([
            rgb_gradient[0],
            rgb_gradient[1],
            rgb_gradient[2],
            0xff,
        ]))
    }

    pub fn set(&mut self, r: u8, g: u8, b: u8, a: u8) {
        self.r = r;
        self.g = g;
        self.b = b;
        self.a = a;
    }

    fn flat(&self) -> [u8; 4] {
        [self.r, self.g, self.b, self.a]
    }
}

pub struct PixelImage {
    pub width: usize,
    pub height: usize,
    pub pixels: Vec<Vec<RgbaPixel>>,
}

impl PixelImage {
    pub fn new(width: usize, height: usize) -> PixelImage {
        PixelImage {
            width,
            height,
            pixels: vec![vec![RgbaPixel::new(None); width]; height],
        }
    }

    pub fn flat(&mut self) -> Vec<u8> {
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
