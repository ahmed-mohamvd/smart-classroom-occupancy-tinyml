import fs from "node:fs/promises";
import { SpreadsheetFile, Workbook } from "@oai/artifact-tool";

const outputDir = "outputs/data_collection_system";
await fs.mkdir(outputDir, { recursive: true });

const workbook = Workbook.create();
const plan = workbook.worksheets.add("Session Plan");
const schema = workbook.worksheets.add("Data Schema");
const font = "Arial";
const navy = "#17365D";
const blue = "#D9EAF7";
const paleBlue = "#EAF3F8";
const amber = "#FFF2CC";
const green = "#E2F0D9";
const red = "#FCE4D6";
const gray = "#E7E6E6";
const text = "#1F2937";

for (const sheet of [plan, schema]) {
  sheet.showGridLines = false;
  sheet.getRange("A1:K60").format.font = { name: font, size: 10, color: text };
  sheet.getRange("A1:K60").format.verticalAlignment = "center";
}

plan.tabColor = navy;
plan.getRange("A2:K2").values = [[
  "Occupancy sensor data collection plan", "", "", "", "", "", "", "", "", "", "",
]];
plan.getRange("A2").format.font = { name: font, size: 14, bold: true, color: navy };
plan.getRange("A3:K3").format.borders = {
  bottom: { style: "thin", color: "#9FBAD0" },
};

plan.getRange("A4:B8").values = [
  ["Sampling interval (seconds)", 1],
  ["Default session duration (minutes)", 10],
  ["Target sessions per class", 5],
  ["Target total sessions", null],
  ["Planned total samples", null],
];
plan.getRange("B7").formulas = [["=B6*3"]];
plan.getRange("B8").formulas = [["=B5*60/B4*B7"]];
plan.getRange("A4:A8").format.fill = paleBlue;
plan.getRange("A4:A8").format.font = { name: font, size: 10, bold: true, color: text };
plan.getRange("A4:A8").format.wrapText = true;
plan.getRange("A4:B8").format.rowHeight = 30;
plan.getRange("B4:B6").format.fill = amber;
plan.getRange("B4:B8").format.numberFormat = "0";
plan.getRange("A4:B8").format.borders = {
  preset: "outside", style: "thin", color: "#9FBAD0",
};

plan.getRange("D4:G4").values = [["Class", "Sessions", "Minutes", "Planned samples"]];
plan.getRange("D5:D8").values = [["EMPTY"], ["LOW"], ["HIGH"], ["TOTAL"]];
plan.getRange("E5").formulas = [["=COUNTIFS($B$12:$B$26,D5)"]];
plan.getRange("E5:E7").fillDown();
plan.getRange("F5").formulas = [["=SUMIFS($E$12:$E$26,$B$12:$B$26,D5)"]];
plan.getRange("F5:F7").fillDown();
plan.getRange("G5").formulas = [["=SUMIFS($I$12:$I$26,$B$12:$B$26,D5)"]];
plan.getRange("G5:G7").fillDown();
plan.getRange("E8").formulas = [["=SUM(E5:E7)"]];
plan.getRange("F8").formulas = [["=SUM(F5:F7)"]];
plan.getRange("G8").formulas = [["=SUM(G5:G7)"]];
plan.getRange("D4:G4").format = {
  fill: navy,
  font: { name: font, size: 10, bold: true, color: "#FFFFFF" },
  horizontalAlignment: "center",
  borders: { preset: "inside", style: "thin", color: "#FFFFFF" },
};
plan.getRange("D5:G8").format.borders = {
  insideHorizontal: { style: "thin", color: "#D6E0E8" },
  bottom: { style: "thin", color: "#9FBAD0" },
};
plan.getRange("D8:G8").format.font = { name: font, size: 10, bold: true, color: text };
plan.getRange("E5:G8").format.numberFormat = "#,##0";

plan.getRange("A10:K10").values = [[
  "Keep the device fixed. Change lighting, occupant position, and movement in a balanced way across all three classes.",
  "", "", "", "", "", "", "", "", "", "",
]];
plan.getRange("A10").format.font = { name: font, size: 10, italic: true, color: "#595959" };

const headers = [
  "Session ID", "Label", "People", "Lighting", "Duration min", "Activity",
  "Position coverage", "Device setup", "Planned samples", "Status", "Notes",
];
plan.getRange("A11:K11").values = [headers];
plan.getRange("A11:K11").format = {
  fill: navy,
  font: { name: font, size: 10, bold: true, color: "#FFFFFF" },
  horizontalAlignment: "center",
  wrapText: true,
  borders: { preset: "inside", style: "thin", color: "#FFFFFF" },
};
plan.getRange("A11:K11").format.rowHeight = 32;

const sessions = [
  ["E01", "EMPTY", 0, "Lights off", 10, "No occupants", "Empty room", "Fixed final mount", null, "Planned", "Close doors after setup"],
  ["E02", "EMPTY", 0, "Normal lights", 10, "No occupants", "Empty room", "Fixed final mount", null, "Planned", "Typical daytime setup"],
  ["E03", "EMPTY", 0, "Bright/daylight", 10, "No occupants", "Empty room", "Fixed final mount", null, "Planned", "Record daylight variation"],
  ["E04", "EMPTY", 0, "Dim light", 10, "No occupants", "Empty room", "Fixed final mount", null, "Planned", "Run at a different time"],
  ["E05", "EMPTY", 0, "Mixed/daylight", 10, "No occupants", "Empty room", "Fixed final mount", null, "Planned", "Final EMPTY test session"],
  ["L01", "LOW", 1, "Normal lights", 10, "Mostly seated", "Near, center, far", "Fixed final mount", null, "Planned", "Change seat during session"],
  ["L02", "LOW", 1, "Lights off", 10, "Walking and seated", "Room coverage", "Fixed final mount", null, "Planned", "Natural motion pattern"],
  ["L03", "LOW", 2, "Bright/daylight", 10, "Mostly seated", "Different seats", "Fixed final mount", null, "Planned", "Avoid blocking sensors"],
  ["L04", "LOW", 2, "Dim light", 10, "Mixed movement", "Near and far", "Fixed final mount", null, "Planned", "Include quiet periods"],
  ["L05", "LOW", 1, "Mixed/daylight", 10, "Mixed movement", "Full room", "Fixed final mount", null, "Planned", "Final LOW test session"],
  ["H01", "HIGH", 3, "Normal lights", 10, "Mostly seated", "Several positions", "Fixed final mount", null, "Planned", "Natural conversation"],
  ["H02", "HIGH", 3, "Lights off", 10, "Mixed movement", "Room coverage", "Fixed final mount", null, "Planned", "Do not crowd one sensor"],
  ["H03", "HIGH", 4, "Bright/daylight", 10, "Mostly seated", "Different seats", "Fixed final mount", null, "Planned", "Include small movements"],
  ["H04", "HIGH", 4, "Dim light", 10, "Walking and seated", "Near and far", "Fixed final mount", null, "Planned", "Natural entry and exit"],
  ["H05", "HIGH", 5, "Mixed/daylight", 10, "Mixed movement", "Full room", "Fixed final mount", null, "Planned", "Final HIGH test session"],
];
plan.getRange("A12:K26").values = sessions;
plan.getRange("I12").formulas = [["=E12*60/$B$4"]];
plan.getRange("I12:I26").fillDown();
plan.getRange("A12:K26").format.borders = {
  insideHorizontal: { style: "thin", color: "#D9E2F3" },
  bottom: { style: "thin", color: "#9FBAD0" },
};
plan.getRange("A12:K26").format.wrapText = true;
plan.getRange("A12:K26").format.rowHeight = 30;
plan.getRange("B12:B26").format.horizontalAlignment = "center";
plan.getRange("C12:E26").format.horizontalAlignment = "center";
plan.getRange("I12:J26").format.horizontalAlignment = "center";
plan.getRange("C12:C26").format.numberFormat = "0";
plan.getRange("E12:E26").format.numberFormat = "0";
plan.getRange("I12:I26").format.numberFormat = "#,##0";
plan.getRange("J12:J26").format.fill = amber;
plan.getRange("J12:J26").dataValidation = {
  rule: { type: "list", values: ["Planned", "In progress", "Complete", "Redo"] },
};
plan.getRange("J12:J26").conditionalFormats.add("containsText", {
  text: "Complete", format: { fill: green, font: { color: "#375623", bold: true } },
});
plan.getRange("J12:J26").conditionalFormats.add("containsText", {
  text: "Redo", format: { fill: red, font: { color: "#9C0006", bold: true } },
});

const planWidths = [22, 10, 9, 18, 13, 22, 22, 19, 15, 13, 26];
for (let i = 0; i < planWidths.length; i += 1) {
  plan.getRangeByIndexes(0, i, 27, 1).format.columnWidth = planWidths[i];
}
plan.freezePanes.freezeRows(11);
plan.freezePanes.freezeColumns(2);

schema.tabColor = "#5B9BD5";
schema.getRange("A2:K2").values = [["Data fields and operating rules", "", "", "", "", "", "", "", "", "", ""]];
schema.getRange("A2").format.font = { name: font, size: 14, bold: true, color: navy };
schema.getRange("A3:K3").format.borders = {
  bottom: { style: "thin", color: "#9FBAD0" },
};

schema.getRange("A4:D4").values = [["Raw CSV field", "Unit/type", "Meaning", "Cleaning rule"]];
schema.getRange("A5:D19").values = [
  ["timestamp", "ISO date-time", "Host time anchored to STM32 uptime", "Must parse as a timestamp"],
  ["run_id", "text", "One logger run", "Used to make sessions unique"],
  ["sequence", "count", "Row number inside session", "Must be numeric"],
  ["mcu_time_ms", "ms", "STM32 uptime", "Kept in raw data"],
  ["session_id", "text", "STM32 session number", "One label per session"],
  ["pir_1", "0/1", "PIR sensor 1 state", "Only 0 or 1"],
  ["pir_2", "0/1", "PIR sensor 2 state", "Only 0 or 1"],
  ["tof_mm", "mm", "VL53L0X distance", "20–2000 mm after status check"],
  ["tof_status", "code", "VL53L0X RangeStatus", "0 is valid; other values are invalid"],
  ["temperature_c", "°C", "BME280 temperature", "−40 to 85 °C"],
  ["humidity_percent", "%RH", "BME280 humidity", "0 to 100%"],
  ["pressure_hpa", "hPa", "BME280 pressure", "300 to 1100 hPa; retained in raw"],
  ["light_lux", "lux", "BH1750 ambient light", "0 to 65,535 lux"],
  ["sample_valid", "0/1", "BME280 and BH1750 read succeeded", "Only rows with 1 are used"],
  ["occupancy_label", "class", "Manual ground truth", "EMPTY, LOW, or HIGH"],
];

schema.getRange("F4:G4").values = [["Training CSV field", "Use"]];
schema.getRange("F5:G13").values = [
  ["timestamp", "Ordering and session timing"],
  ["session_id", "Independent train/validation/test split"],
  ["pir_1", "Current PIR state"],
  ["pir_2", "Current PIR state"],
  ["tof_mm", "Valid or short-gap-filled distance"],
  ["temperature_c", "Environmental feature"],
  ["humidity_percent", "Environmental feature"],
  ["light_lux", "Environmental feature"],
  ["occupancy_label", "Target class"],
];

schema.getRange("I4:J4").values = [["Command", "Action"]];
schema.getRange("I5:J10").values = [
  ["E", "Select EMPTY before recording"],
  ["L", "Select LOW (1–2 people)"],
  ["H", "Select HIGH (3+ people)"],
  ["S", "Start a new session"],
  ["X", "Stop the current session"],
  ["?", "Print status and sensor readiness"],
];

schema.getRange("F16:J16").values = [["Quality rule", "", "", "", ""]];
schema.getRange("F17:J23").values = [
  ["Sampling", "1 synchronized row per second", "", "", ""],
  ["Minimum coverage", "At least 3 sessions per class; 5 preferred", "", "", ""],
  ["ToF invalid", "Status other than 0 or 8190/8191 mm is not a distance", "", "", ""],
  ["Short ToF gap", "Forward-fill at most 3 consecutive samples", "", "", ""],
  ["Mounting", "Keep device position and angle fixed across normal sessions", "", "", ""],
  ["Class change", "Stop, select the new label, then start a new session", "", "", ""],
  ["Optional filter", "Causal median only; deployment must use the same filter", "", "", ""],
];

for (const headerRange of ["A4:D4", "F4:G4", "I4:J4", "F16:J16"]) {
  schema.getRange(headerRange).format = {
    fill: navy,
    font: { name: font, size: 10, bold: true, color: "#FFFFFF" },
    horizontalAlignment: "center",
    borders: { preset: "inside", style: "thin", color: "#FFFFFF" },
  };
}
for (const bodyRange of ["A5:D19", "F5:G13", "I5:J10", "F17:J23"]) {
  schema.getRange(bodyRange).format.borders = {
    insideHorizontal: { style: "thin", color: "#D9E2F3" },
    bottom: { style: "thin", color: "#9FBAD0" },
  };
  schema.getRange(bodyRange).format.wrapText = true;
}
schema.getRange("A5:A19").format.fill = paleBlue;
schema.getRange("F5:F13").format.fill = paleBlue;
schema.getRange("I5:I10").format.fill = gray;
schema.getRange("A5:J23").format.rowHeight = 28;
schema.getRange("A4:J4").format.rowHeight = 28;
schema.getRange("F16:J16").format.rowHeight = 26;

const schemaWidths = [22, 14, 30, 38, 3, 23, 35, 3, 12, 38, 3];
for (let i = 0; i < schemaWidths.length; i += 1) {
  schema.getRangeByIndexes(0, i, 25, 1).format.columnWidth = schemaWidths[i];
}
schema.freezePanes.freezeRows(4);

workbook.recalculate();

const planCheck = await workbook.inspect({
  kind: "table",
  range: "Session Plan!A2:K26",
  include: "values,formulas",
  tableMaxRows: 30,
  tableMaxCols: 12,
});
console.log(planCheck.ndjson);
const schemaCheck = await workbook.inspect({
  kind: "table",
  range: "Data Schema!A2:J23",
  include: "values,formulas",
  tableMaxRows: 24,
  tableMaxCols: 10,
});
console.log(schemaCheck.ndjson);
const errors = await workbook.inspect({
  kind: "match",
  searchTerm: "#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A|#NUM!|#NULL!|#SPILL!|#CALC!",
  options: { useRegex: true, maxResults: 100 },
  summary: "final formula error scan",
});
console.log(errors.ndjson);

for (const sheetName of ["Session Plan", "Data Schema"]) {
  const preview = await workbook.render({
    sheetName,
    autoCrop: "all",
    scale: 1,
    format: "png",
  });
  const previewName = sheetName.toLowerCase().replaceAll(" ", "_");
  await fs.writeFile(
    `${outputDir}/${previewName}.png`,
    new Uint8Array(await preview.arrayBuffer()),
  );
}

const output = await SpreadsheetFile.exportXlsx(workbook);
await output.save(`${outputDir}/data_collection_plan.xlsx`);
