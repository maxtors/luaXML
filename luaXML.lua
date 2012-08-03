---- PARSING XML ----
file = "testdata.xml"


-- PROGRAM STARTS HERE

print("File:", file)
x = os.clock()
result = parseXML(file)
print(string.format("elapsed time: %.2f\n", os.clock() - x))

if result == nil then
  print("Error parseing XML")
else
  --printResult(result)
  print("Name:", result.name)
  print("Attr:", result.attr)
  print("Tags:", result.tags)
  print("Data:", result.data)
end
