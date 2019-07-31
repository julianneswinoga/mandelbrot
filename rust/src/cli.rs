use num_complex::Complex;
use regex::Regex;
use structopt::StructOpt;

use crate::calculations::FloatPrecision;

fn parse_starting_scale(
    src: &str,
) -> Result<(Complex<FloatPrecision>, Complex<FloatPrecision>), String> {
    let re = Regex::new(r"\D").unwrap();

    let substrs: Vec<&str> = src
        .trim_matches(|c: char| re.is_match(&c.to_string()))
        .split(',')
        .collect();
    if substrs.len() != 4 {
        return Err(format!("must only have 4 elements"));
    }

    let mut floats = Vec::<FloatPrecision>::new();
    for substr in substrs {
        floats.push(match substr.trim().parse::<FloatPrecision>() {
            Ok(f) => f,
            Err(e) => return Err(format!("{}: Could not parse {:?}", substr, e)),
        })
    }

    let default_top_left_scale = Complex::<FloatPrecision>::new(floats[0], floats[1]);
    let default_bottom_right_scale = Complex::<FloatPrecision>::new(floats[2], floats[3]);

    Ok((default_top_left_scale, default_bottom_right_scale))
}

#[derive(StructOpt, Debug)]
#[structopt(
    name = "mandelbrot",
    about = "a rusty implementation of a mandelbrot viewer"
)]
pub struct Opt {
    // The number of occurrences of the `v/verbose` flag
    /// Verbose mode (-v, -vv, -vvv, etc.)
    #[structopt(short = "v", long = "verbose", parse(from_occurrences))]
    pub verbose: u8,

    #[structopt(
        long = "starting_scale",
        help = "Set the starting screen to starting_scale. Coords are left,top,right,bottom",
        default_value = "(-2.5,-1.0,1.0,1.0)",
        parse(try_from_str = "parse_starting_scale")
    )]
    pub starting_scale: (Complex<FloatPrecision>, Complex<FloatPrecision>),
}
