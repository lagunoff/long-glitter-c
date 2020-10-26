Driver :: {
  x1 :: Int,
  x2 :: String,
  x3 :: Date,
}

Car :: x y z => {
  x1 :: Int,
  x2 :: String,
  x3 :: Date,
}

proc02 :: (x: Int) => {
  one = 1, two = 2, three;
}

proc03 ::
  a::Int
  b::String
  c::Date
  => do
    x <- blala
    y <- blabla
    return {x,y},y};