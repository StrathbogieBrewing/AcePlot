"use strict";

var logData;
var logSources;

function boxChecked(name){
  let element = document.getElementById(name);
  if (element != null){
    if (element.checked){
      return true;
    }
  }
  return false;
}

function plotChangeEvent(ev){
  let sources = logSources.filter(boxChecked);
  plotData(sources);
}

function plotData(sources){
  let timeData = [];
  logData.forEach(rec => {
    let data = [];
    sources.forEach(source => {
      let value = rec[source];
      if(value == undefined){
        data.push(null);
      } else {
        data.push(value);
      }
    });
    timeData.push(data);
  })

  let graph = new Dygraph(document.getElementById("graphdiv"), timeData, {
    labels : sources,
    connectSeparatedPoints: true,
    drawPoints: true,
    axes : {
      x : {
        axisLabelFormatter : function(
            d, gran) { return new Date(1 * d).toLocaleTimeString(); },
        valueFormatter : function(
            s) { return new Date(1 * s).toLocaleString('en-AU'); }
      }
    }
  });
}

function checkBox(name, enc) {
  let content = document.createElement(enc);
  let checkbox = document.createElement('input');
  checkbox.setAttribute("type", "checkbox");
  checkbox.setAttribute("id", name);
  checkbox.addEventListener('change', (ev) => {
    plotChangeEvent(ev);
  });
  content.append(checkbox);
  let label = document.createElement('label');
  label.setAttribute("for", name);
  label.textContent = name;
  content.append(label);
  return content;
}

async function getData() {
  let timeData = [];
  await fetch('cgi-bin/logger?duration=86400').then(response => response.json()).then(data => {
    logData = data.TimeSeries;
    let content = document.createElement('div');
    content.setAttribute("id", "datadiv");
    let checkBoxGrid = document.createElement('ul');
    checkBoxGrid.setAttribute("class", 'checkbox-grid');
    logSources = [];
    logData.forEach(rec => {
      Object.keys(rec).forEach(k => {
        if (logSources.indexOf(k) == -1) {
          logSources.push(k);
          let checkbox = checkBox(k, 'li');
          if(k == "t"){
            checkbox.firstChild.checked = true;
            checkbox.firstChild.disabled = true;
          }
          checkBoxGrid.append(checkbox);
        }
      });
    });
    content.append(checkBoxGrid);
    document.querySelector('#datadiv').replaceWith(content);
    plotChangeEvent();
  });
}

getData();


// let valueKeys = [];
// logData.forEach(r => {
//   Object.keys(r).forEach(k => {
//     if(valueKeys.indexOf(k) == -1){
//       valueKeys.push(k);
//     }
//   });
// })

// console.log(logData);


      // Object.keys(r).forEach(k => {
      //   if(valueKeys.indexOf(k) == -1){
      //     valueKeys.push(k);
      //   }
      // });

  // var vbat;
  // var tbat;
  // var duration = '7200'
  // await fetch('cgi-bin/api?source=vbat;duration=1440').then(response =>
  // response.json().then(data => vbat = data)); await
  // fetch('cgi-bin/api?source=tbat;duration=1440').then(response =>
  // response.json().then(data => tbat = data));
  //
  // const fetches = [
  //   fetch('cgi-bin/api?source=vbat;duration=' + duration).then(response =>
  //   response.json().then(data => vbat = data)),
  //   fetch('cgi-bin/api?source=tbat;duration=' + duration).then(response =>
  //   response.json().then(data => tbat = data))
  // ];
  //
  // await Promise.all(fetches);
