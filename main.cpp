#include <string>
#include <vector>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);

void line2(Vec2i v0, Vec2i v1, TGAImage& image, TGAColor color) {
  for (float t = 0.; t < 1.; t += .1) {
    int x = v0.x * (1. - t) + v1.x * t;
    int y = v0.y * (1. - t) + v1.y * t;
    image.set(x, y, color);
  }
}

void line(Vec2i v0, Vec2i v1, TGAImage& image, const TGAColor& color) {
  bool is_range_y = false;

  // We want to do work on a range that is longer than the domain so each pixel has a value.
  if (std::abs(v1.y - v0.y) > std::abs(v1.x - v0.x)) {
    is_range_y = true;
    std::swap(v0.x, v0.y);
    std::swap(v1.x, v1.y);
  }

  if (v0.x > v1.x) {
    std::swap(v0, v1);
  }

  int range = v1.x - v0.x;

  // If points are same exit.
  if (range == 0) {
    image.set(v0.x, v0.y, color);
    return;
  }

  // v0.x is range_start, v1.x is range_end, v0.y is domain_start, v1.y is domain_end.
  // Keep in mind v1.y - v0.y < v1.x - v0.x.
  // Therefore, (v1.y - v0.y) / (v1.x - v0.x) < 1.
  float domain_range_inverse = (v1.y - v0.y) * (1.0f / range);

  // If range is 1, we have (range + 1) or two pixels.
  float domain_closest = v0.y;
  for (int i = v0.x; i <= v1.x; i++) {
    if (is_range_y) {
      image.set(domain_closest, i, color);
    } else {
      image.set(i, domain_closest, color);
    }
    domain_closest += domain_range_inverse;
  }
}

void renderLines() {
  const int width = 500;
  const int height = 500;

  TGAImage image(width, height, TGAImage::RGB);
  std::vector<int> start = {250, 250};
  std::vector<int> end = {80, 41};
  std::vector<int> x_series = {end[0], end[1], -end[1], -end[0], -end[0], -end[1], end[1], end[0]};
  std::vector<int> y_series = {end[1], end[0], end[0], end[1], -end[1], -end[0], -end[0], -end[1]};

  // This is for profiling purposes.
  for (int j = 0; j < 1e5; j++) {
    for (size_t i = 0; i < x_series.size(); i++) {
      line(
          Vec2i(start[0], start[1]),
          Vec2i(start[0] + x_series[i], start[1] + y_series[i]),
          image,
          TGAColor(0xff - (i * 20), 0xff - (i * 20), 0xff - (i * 20), 0xff));
    }
  }

  image.flip_vertically();  // i want to have the origin at the left bottom corner of the image
  image.write_tga_file("output.tga");
}

void renderModel() {
  std::string kModelFileName = "obj/african_head/african_head.obj";
  std::unique_ptr<Model> model = std::make_unique<Model>(kModelFileName.c_str());
  const int width = 800;
  const int height = 600;

  TGAImage image(width, height, TGAImage::RGB);
  for (int i = 0; i < model->nfaces(); i++) {
    std::vector<int> face = model->face(i);
    for (int j = 0; j < 3; j++) {
      Vec3f v0 = model->vert(face[j]);
      Vec3f v1 = model->vert(face[(j + 1) % 3]);
      int x0 = (v0.x + 1.) * width / 2.;
      int y0 = (v0.y + 1.) * height / 2.;
      int x1 = (v1.x + 1.) * width / 2.;
      int y1 = (v1.y + 1.) * height / 2.;
      line(Vec2i(x0, y0), Vec2i(x1, y1), image, white);
    }
  }

  image.flip_vertically();  // i want to have the origin at the left bottom corner of the image
  image.write_tga_file("output2.tga");
}

void triangle_unfilled(std::vector<Vec2i> v, TGAImage& image, const TGAColor& color) {
  line(v[0], v[1], image, color);
  line(v[1], v[2], image, color);
  line(v[2], v[0], image, color);
}

void triangle(std::vector<Vec2i> vertices, TGAImage& image, const TGAColor& color) {
  Vec2i v0 = vertices[0];
  Vec2i v1 = vertices[1];
  Vec2i v2 = vertices[2];

  // If all 3 points on a line, then not a triangle return.
  if ((v0.x == v1.x && v1.x == v2.x) || (v0.y == v1.y && v1.y == v2.y)) {
    // TODO: Check for invalid triangles along diagonals or maybe algorithm works.
    return;
  }

  // Sort vertices by y, v0 highest y
  if (v0.y < v1.y) {
    std::swap(v0, v1);
  }
  if (v1.y < v2.y) {
    std::swap(v1, v2);
  }

  if (v0.y < v1.y) {
    std::swap(v0, v1);
  }

  // Rasterize simultaneously left and right of triangle.
  // Draw horizontal line segment between left and right boundary points.
  float delta_v0_v1_inverse = static_cast<float>(v1.x - v0.x) / static_cast<float>(v1.y - v0.y);
  float delta_v0_v2_inverse = static_cast<float>(v2.x - v0.x) / static_cast<float>(v2.y - v0.y);
  float delta_v1_v2_inverse = static_cast<float>(v2.x - v1.x) / static_cast<float>(v2.y - v1.y);

  int from_x;
  int to_x;
  for (int y = v0.y; y >= v2.y; y--) {
    if (y >= v1.y) {
      from_x = v0.x + delta_v0_v1_inverse * (y - v0.y);
      to_x = v0.x + delta_v0_v2_inverse * (y - v0.y);
    } else {
      from_x = v2.x + delta_v0_v2_inverse * (y - v2.y);
      to_x = v2.x + delta_v1_v2_inverse * (y - v2.y);
    }

    if (from_x > to_x) {
      std::swap(from_x, to_x);
    }

    for (int x = from_x; x <= to_x; x++) {
      image.set(x, y, color);
    }
  }
}

// Use bounding box and then check whether point is in triangle.
// void triangle_bb(Vec2i v0, Vec2i v1, Vec2i v2, TGAImage& image, const TGAColor& color) {
// }

void renderTriangles() {
  const int width = 200;
  const int height = 200;
  TGAImage image(width, height, TGAImage::RGB);

  std::vector<Vec2i> t0 = {Vec2i(10, 70), Vec2i(50, 160), Vec2i(70, 80)};
  std::vector<Vec2i> t1 = {Vec2i(180, 50), Vec2i(150, 1), Vec2i(70, 180)};
  std::vector<Vec2i> t2 = {Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180)};

  triangle_unfilled(t0, image, white);
  for(int i = 0; i < 1e4; i++) {
    triangle(t0, image, red);
    triangle(t1, image, white);
    triangle(t2, image, green);
  }

  image.flip_vertically();  // i want to have the origin at the left bottom corner of the image
  image.write_tga_file("output3.tga");
}

int main() {
  // renderLines();
  // renderModel();
  renderTriangles();
  return 0;
}
