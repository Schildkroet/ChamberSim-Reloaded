local admonition_titles = {
  warning = "Safety",
  note = "Note",
}

function Image(image)
  if not image.src:match("^[%a][%w+.-]*:") and
     not image.src:match("^/") and
     not image.src:match("^images/") then
    image.src = "images/" .. image.src
  end
  return image
end

function Table(table)
  return pandoc.RawBlock(
    "gfm",
    pandoc.write(pandoc.Pandoc({table}), "gfm")
  )
end

function Span(span)
  if span.classes:includes("sans-serif") then
    return span.content
  end
end

function Math(math)
  if math.text == "\\triangleright" then
    return pandoc.Str("→")
  end

  if math.mathtype == "InlineMath" then
    return pandoc.RawInline("gfm", "$" .. math.text .. "$")
  end
end

function Header(header)
  if header.level == 4 then
    header.level = 3
  end

  local last_inline = header.content[#header.content]
  if last_inline and last_inline.t == "Str" then
    last_inline.text = last_inline.text:gsub("%.+$", "")
  end

  return header
end

function Div(div)
  for class, title in pairs(admonition_titles) do
    if div.classes:includes(class) then
      local gfm = pandoc.write(pandoc.Pandoc(div.content), "gfm")
      local quoted_lines = {}
      local content = gfm:gsub("\n+$", "")
      for line in (content .. "\n"):gmatch("(.-)\n") do
        quoted_lines[#quoted_lines + 1] = "> " .. line
      end
      return pandoc.RawBlock(
        "gfm",
        "> [!" .. class:upper() .. "]\n" .. table.concat(quoted_lines, "\n")
      )
    end
  end

  if div.classes:includes("center") then
    return div.content
  end
end
