// API: canvas.add_tab/add_submenu and every MenuBuilder method.
import * as canvas from "canvas";

canvas.add_tab("TypeScript example", 52001, (menu) => {
  menu.add_bool("Example enabled", "Example toggle");
  menu.add_slider("Example amount", 0, 10, 0.5);
  menu.add_choose("Example mode", ["First", "Second"], (index, value) => {
    console.log("Selected mode:", index, value);
  });
  menu.add_option("Print message", () => console.log("Option selected"));
  // This binding currently registers the submenu but its renderer skips a new
  // submenu ID, so the entry may not appear until the native version bug is fixed.
  menu.add_submenu("More examples", 52002, () => console.log("Submenu opened"));
});

canvas.add_submenu("More examples", 52002, (menu) => {
  menu.add_bool("Extra option");
  menu.add_option("Return message", () => console.log("Extra option selected"));
});
