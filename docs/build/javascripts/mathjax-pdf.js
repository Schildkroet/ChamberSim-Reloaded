window.MkDocsExporter = window.MkDocsExporter || {};
window.MkDocsExporter.render = async () => {
  await window.MathJax.startup.promise;
  await window.MathJax.typesetPromise();
};
