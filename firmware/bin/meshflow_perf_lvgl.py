"""Apply the measured M9 LVGL compiler profile to the graphics hot paths."""

Import("env")

if env.subst("$PIOENV") != "thinknode_m9-tft":
    print("MeshFlow LVGL profile: skipped for %s" % env.subst("$PIOENV"))
else:
    iram_sources = {"lv_draw_sw_blend.c", "lv_draw_label.c"}
    iram_header = env.subst("$PROJECT_DIR") + "/bin/meshflow_lvgl_iram.h"

    def meshflow_lvgl_object(build_env, node):
        flags = list(build_env["CCFLAGS"])
        flags = [flag for flag in flags if flag not in ("-Os", "-O1", "-O3")]
        flags.append("-O2")
        if node.name in iram_sources:
            flags.extend(("-include", iram_header))
        return build_env.Object(node, CCFLAGS=flags)

    env.AddBuildMiddleware(meshflow_lvgl_object, "*lvgl/src/*")
    print("MeshFlow LVGL profile: -O2; IRAM: lv_draw_sw_blend.c, lv_draw_label.c")
