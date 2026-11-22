-- 建立共用的線段、節點與文字圖元。
return function(options)
    local drawing = {}
    function drawing.node(name, parent, position, parameters, controllers)
        local element = CreateElement("ceSimple")
        element.name = name
        element.parent_element = parent
        element.init_pos = position or { 0, 0, 0 }
        element.element_params = parameters
        element.controllers = controllers
        element.collimated = options.collimated
        if options.screenspace then element.screenspace = ScreenType.SCREENSPACE_TRUE end
        Add(element)
        return element.name
    end
    function drawing.line(name, parent, x1, y1, x2, y2, thickness, material, parameters, controllers)
        local dx, dy = x2 - x1, y2 - y1
        local length = math.sqrt(dx * dx + dy * dy)
        local nx, ny = -dy / length * thickness / 2, dx / length * thickness / 2
        local element = CreateElement("ceMeshPoly")
        element.name = name
        element.parent_element = parent
        element.primitivetype = "triangles"
        element.material = material
        element.vertices = { { x1 - nx, y1 - ny }, { x1 + nx, y1 + ny },
            { x2 + nx, y2 + ny }, { x2 - nx, y2 - ny } }
        element.indices = { 0, 1, 2, 0, 2, 3 }
        element.element_params = parameters
        element.controllers = controllers
        element.collimated = options.collimated
        -- 前視線段以自身描繪層發布，與座艙其他裁切遮罩共同顯示。
        if options.clip_relation then
            element.h_clip_relation = options.clip_relation
            element.level = options.level
        end
        element.use_mipfilter = true
        if options.screenspace then element.screenspace = ScreenType.SCREENSPACE_TRUE end
        Add(element)
        return element
    end
    function drawing.box(name, parent, half_width, half_height, thickness, material)
        drawing.line(name .. "_top", parent, -half_width, half_height, half_width, half_height, thickness, material)
        drawing.line(name .. "_bottom", parent, -half_width, -half_height, half_width, -half_height, thickness, material)
        drawing.line(name .. "_left", parent, -half_width, -half_height, -half_width, half_height, thickness, material)
        drawing.line(name .. "_right", parent, half_width, -half_height, half_width, half_height, thickness, material)
    end
    function drawing.text(name, parent, value, position, size, material, parameter, format)
        local element = CreateElement("ceStringPoly")
        element.name = name
        element.parent_element = parent
        element.material = material
        element.value = value
        element.alignment = "LeftTop"
        element.stringdefs = { size, size, 0, 0 }
        element.init_pos = position
        if parameter then
            element.element_params = { parameter }
            element.controllers = { { "text_using_parameter", 0, 0 } }
            element.formats = { format }
        end
        element.collimated = options.collimated
        if options.screenspace then element.screenspace = ScreenType.SCREENSPACE_TRUE end
        Add(element)
    end
    return drawing
end
