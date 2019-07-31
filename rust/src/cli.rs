use structopt::StructOpt;

use crate::calculations::FloatPrecision;

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
    pub starting_scale: [FloatPrecision; 4],
}
