const express = require('express');
const app = express();
const port = 8000;

// Mock data; later replace with real sensor readings
let history = [
  [Date.now() - 60000*5, 20],
  [Date.now() - 60000*4, 22],
  [Date.now() - 60000*3, 21.5],
  [Date.now() - 60000*2, 23],
  [Date.now() - 60000*1, 22.8]
];

app.use(express.json());

app.get('/api/level', (req, res) => {
  const current = history[history.length - 1][1];
  const timestamp = history[history.length - 1][0];
  res.json({ current, timestamp, history });
});

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}/`);
});
