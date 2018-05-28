import DataManager from '../api/DataManager';

// Function for converting the focus node list to an array
export const fromStringToArray = string => string.replace(/['"]+/g, '').split(', ');

// Function to toggle the shading on a planet, value = 'true' equals shading enabled
export const toggleShading = (planet, value) => {
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.${planet}.RenderableGlobe.PerformShading", ${value})`);
};

// Function to toggle high resolution data for a planet, data of type CTX_Mosaic
// value = 'true' equals high resolution data enabled
export const toggleHighResolution = (planet, value) => {
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.${planet}.RenderableGlobe.Layers.ColorLayers.CTX_Mosaic_Utah.Enabled", ${value})`);
};

// Function to toggle the visibility of a planet,
// value = 'true' equals the planet being visible and not hidden
export const toggleHidePlanet = (planet, value) => {
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.${planet}.RenderableGlobe.Enabled", ${value})`);
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.${planet}Trail.renderable.Enabled",${value})`);
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.${planet}.ScreenVisibility",${value})`);
};

// Function to toggle the visibility of galaxies, value = 'true' equals galaxies enabled
export const toggleGalaxies = (value) => {
  DataManager.runScript(`openspace.setPropertyValueSingle("Scene.SloanDigitalSkySurvey.renderable.Enabled", ${value})`);
};

// Function to zoom out once a story is picked, value = 'true' equals overview enabled,
// anda a slower velocity is used
export const toggleZoomOut = (value) => {
  const velocity = (value === 'true') ? 0.02 : 0.04;
  DataManager.runScript(`openspace.setPropertyValueSingle('NavigationHandler.OrbitalNavigator.VelocityZoomControl', ${velocity})`);
  DataManager.runScript(`openspace.setPropertyValueSingle("NavigationHandler.OrbitalNavigator.Overview", ${value})`);
};

// Function to reset the bool properties to the default value
export const resetBoolProperty = (URI, value) => {
  DataManager.runScript(`openspace.setPropertyValueSingle("${URI}", ${value})`);
};